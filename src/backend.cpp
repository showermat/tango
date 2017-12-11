/* 
 * File:   backend.cpp
 * Author: Matthew Schauer
 *
 * Created on July 27, 2013, 5:46 PM
 */

#include "backend.h"
#include "Bank.h"
#include "Card.h"
#include "Deck.h"

namespace backend
{
	std::string deckfname{};
	sqlite3 *db = nullptr;
	
	std::time_t midnight()
	{
		std::time_t now = std::time(nullptr);
		std::tm *ret = std::localtime(&now);
		ret->tm_sec = ret->tm_min = ret->tm_hour = 0;
		return std::mktime(ret);
	}
	
	void cleanup()
	{
		if (db != nullptr) sqlite3_close(db);
		db = nullptr;
	}
	
	std::string deckfile()
	{
		std::string pathsep{"/"};
		std::string confdir{"tango"};
		std::string confdb{"decks.db"};
		std::string confbase{};
		char *confhome = getenv("XDG_CONFIG_HOME");
		if (confhome) confbase = confhome;
		else
		{
			char *home = getenv("HOME");
			confbase = std::string{home} + pathsep + ".config";
			// TODO If .config doesn't exit, then use ~/.tango instead
		}
		return confbase + pathsep + confdir + pathsep + confdb;
	}
	
	void batch(const std::vector<std::string> &args) try
	{
		if (args.size() < 3) throw std::runtime_error{"No action provided for batch processing"};
		if (args[2] == "debug")
		{
			if (args[3] == "load")
			{
				populate();
				std::cout << Deck::root.ncards() << " " << Card::cards().size() << "\n";
				commit();
				exit(0);
			}
		}
		else throw std::runtime_error{"Unknown batch operation " + args[2]};
		exit(0); // TODO Probably the wrong way to do this
	}
	catch (std::runtime_error &e)
	{
		std::cerr << "Error: " << e.what() << "\n";
		cleanup();
		exit(1);
	}
	
	void checksql(int code, std::string msg = "Query failed")
	{
		if (code == SQLITE_OK || code == SQLITE_DONE) return;
		msg += ": error " + util::t2s(code) + " (" + std::string{sqlite3_errstr(code)} + "): " + std::string{sqlite3_errmsg(db)};
		throw std::runtime_error{msg};
	}

	// Standin for main.  Call me to initialize stuff!
	void init(const std::vector<std::string> &args) try
	{
		deckfname = deckfile();
		if (! util::file_exists(deckfname)) db_setup(deckfname);
		checksql(sqlite3_open(deckfname.c_str(), &db), "Couldn't open deck database");
		if (args.size() > 1)
		{
			if (args[1] == "batch") batch(args);
			else throw std::runtime_error{"Unknown argument " + args[1]};
		}
	}
	catch (std::runtime_error e)
	{
		cleanup();
		throw;
	}

	void db_setup(const std::string &fname)
	{
		std::vector<std::string> schema{
			"CREATE TABLE \"info\" (`version` INTEGER, `step` INTEGER, `laststep` INTEGER)",
			"CREATE TABLE \"prefs\" (`autostep` INTEGER)",
			"CREATE TABLE \"card\" ( `id` INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, `deck` TEXT NOT NULL, `step` INTEGER NOT NULL DEFAULT 1, `interval` INTEGER NOT NULL DEFAULT 0, `status` INTEGER, `upd_norm` INTEGER NOT NULL DEFAULT 0, `upd_decr` INTEGER NOT NULL DEFAULT 0, `upd_incr` INTEGER NOT NULL DEFAULT 0, `upd_reset` INTEGER NOT NULL DEFAULT 0 )",
			"CREATE TABLE \"character\" ( `deck` TEXT NOT NULL, `category` TEXT, `character` TEXT NOT NULL, `active` INTEGER NOT NULL, `step` INTEGER, `count` INTEGER NOT NULL DEFAULT 0, PRIMARY KEY(deck,character), FOREIGN KEY(`deck`) REFERENCES deck ( id ), FOREIGN KEY(`category`) REFERENCES kanji_category ( name ) )",
			"CREATE TABLE \"character_category\" ( `deck` TEXT NOT NULL, `name` TEXT NOT NULL, PRIMARY KEY(deck,name) )",
			"CREATE TABLE \"deck\" ( `name` TEXT NOT NULL, PRIMARY KEY(name) )",
			"CREATE TABLE \"field\" ( `card` INTEGER NOT NULL, `field` TEXT NOT NULL, `value` TEXT, PRIMARY KEY(card,field), FOREIGN KEY(card) REFERENCES card(id), FOREIGN KEY(field) REFERENCES field(id) )",
			"CREATE TABLE \"fieldname\" ( `name` TEXT NOT NULL, PRIMARY KEY(name) )"
		};
		std::vector<std::string> fieldnames{"Expression", "Reading", "Meaning"};
		std::unordered_map<std::string, std::vector<std::string>> characters{
			{"First grade", {"百", "大", "耳", "一", "正", "王", "早", "川", "森", "車", "見", "口", "虫", "先", "石", "円", "三", "年", "山", "金", "左", "出", "校", "学", "木", "気", "雨", "名", "立", "竹", "女", "休", "男", "村", "入", "千", "青", "文", "目", "子", "九", "林", "白", "日", "夕", "玉", "赤", "十", "町", "下", "生", "五", "四", "糸", "花", "田", "貝", "音", "水", "小", "六", "二", "七", "火", "空", "天", "上", "中", "力", "月", "人", "手", "土", "本", "字", "犬", "八", "草", "足", "右"}},
			{"Second grade", {"作", "角", "夜", "画", "買", "母", "答", "読", "国", "丸", "室", "汽", "谷", "鳴", "午", "南", "風", "歩", "戸", "昼", "雪", "計", "黄", "場", "友", "弟", "書", "野", "体", "通", "顔", "絵", "理", "兄", "船", "考", "半", "公", "何", "家", "多", "話", "思", "才", "米", "毎", "言", "岩", "遠", "星", "京", "知", "来", "形", "間", "用", "秋", "算", "数", "社", "前", "後", "行", "高", "馬", "売", "語", "声", "毛", "曜", "頭", "明", "太", "切", "外", "走", "組", "西", "市", "店", "止", "原", "電", "弓", "長", "寺", "教", "光", "麦", "雲", "万", "科", "刀", "姉", "道", "番", "古", "肉", "今", "会", "強", "少", "茶", "心", "新", "紙", "帰", "父", "自", "台", "東", "合", "妹", "活", "時", "園", "内", "色", "図", "方", "直", "元", "分", "聞", "楽", "線", "親", "黒", "矢", "近", "羽", "引", "牛", "点", "当", "晴", "記", "首", "海", "食", "同", "工", "交", "春", "魚", "池", "鳥", "細", "里", "歌", "北", "週", "回", "地", "夏", "門", "弱", "朝", "冬", "広"}},
			{"Third grade", {"島", "動", "者", "皿", "世", "反", "港", "役", "服", "流", "意", "深", "具", "落", "葉", "調", "化", "相", "秒", "炭", "暗", "集", "暑", "軽", "送", "真", "写", "旅", "都", "住", "注", "急", "豆", "号", "起", "族", "究", "予", "向", "酒", "寒", "研", "宮", "和", "油", "次", "使", "物", "湖", "対", "部", "代", "速", "育", "美", "局", "度", "運", "問", "歯", "鉄", "県", "銀", "区", "係", "感", "終", "安", "平", "定", "階", "波", "主", "温", "発", "練", "曲", "投", "路", "去", "洋", "第", "商", "有", "飲", "駅", "君", "両", "陽", "勉", "界", "登", "死", "味", "仕", "申", "助", "期", "全", "血", "命", "負", "等", "緑", "放", "植", "泳", "配", "談", "悪", "実", "礼", "守", "農", "院", "持", "決", "氷", "州", "業", "転", "身", "昔", "屋", "事", "開", "他", "受", "薬", "打", "乗", "表", "病", "丁", "根", "苦", "消", "横", "畑", "板", "取", "式", "重", "習", "勝", "客", "員", "待", "幸", "漢", "筆", "列", "倍", "始", "医", "館", "返", "想", "球", "神", "様", "題", "短", "遊", "悲", "進", "宿", "品", "由", "拾", "箱", "所", "息", "指", "着", "面", "詩", "福", "帳", "岸", "橋", "鼻", "皮", "祭", "整", "央", "庫", "委", "柱", "昭", "坂", "羊", "追", "級", "庭", "荷", "笛", "湯", "童", "章"}},
			{"Fourth grade", {"加", "位", "別", "完", "不", "挙", "失", "類", "差", "司", "共", "冷", "建", "成", "結", "仲", "漁", "信", "札", "念", "便", "選", "士", "達", "料", "愛", "以", "静", "歴", "令", "救", "約", "昨", "欠", "例", "残", "産", "利", "紀", "堂", "観", "参", "給", "種", "最", "夫", "府", "菜", "満", "英", "機", "梅", "季", "説", "働", "録", "億", "泣", "各", "史", "敗", "未", "然", "労", "付", "的", "伝", "験", "連", "害", "関", "単", "功", "帯", "松", "果", "席", "覚", "省", "械", "側", "議", "停", "節", "束", "象", "費", "熱", "治", "囲", "求", "必", "試", "好", "量", "得", "初", "課", "変", "積", "良", "飯", "改", "末", "続", "笑", "案", "健", "告", "無", "借", "法", "特", "卒", "低", "要", "康", "固", "唱", "浅", "民", "胃", "氏", "隊", "芸", "浴", "印", "旗", "貨", "栄", "願", "極", "臣", "置", "順", "辞", "争", "脈", "刷", "賞", "底", "輪", "芽", "勇", "焼", "航", "貯", "型", "典", "包", "飛", "副", "陸", "清", "官", "塩", "兵", "軍", "器", "郡", "衣", "倉", "訓", "候", "喜", "兆", "材", "徒", "標", "巣", "博", "径", "鏡", "望", "察", "養", "照", "戦", "票", "街", "灯", "腸", "祝", "協", "景", "折", "孫", "競", "老", "辺", "粉", "希", "努", "児", "殺", "周", "管", "毒", "散", "牧"}},
			{"Fifth grade", {"招", "雑", "提", "再", "示", "保", "仏", "豊", "禁", "素", "絶", "酸", "測", "条", "容", "授", "講", "迷", "輸", "報", "効", "確", "複", "防", "術", "限", "衛", "技", "険", "常", "興", "仮", "適", "勢", "接", "制", "居", "経", "件", "弁", "祖", "留", "貸", "断", "在", "現", "張", "統", "復", "識", "質", "破", "能", "責", "比", "略", "規", "逆", "減", "応", "義", "製", "可", "券", "像", "性", "過", "状", "任", "団", "増", "幹", "均", "液", "似", "圧", "厚", "格", "資", "査", "設", "編", "燃", "非", "準", "情", "易", "率", "述", "因", "解", "混", "政", "個", "境", "恩", "枝", "肥", "舎", "精", "則", "綿", "寄", "程", "銅", "構", "敵", "額", "耕", "貧", "判", "許", "承", "務", "領", "群", "態", "舌", "演", "志", "採", "謝", "賀", "財", "潔", "墓", "預", "桜", "快", "銭", "移", "余", "造", "修", "総", "眼", "属", "徳", "師", "序", "永", "績", "飼", "護", "旧", "故", "災", "富", "基", "際", "武", "税", "営", "刊", "織", "賛", "独", "妻", "句", "退", "往", "布", "久", "犯", "河", "鉱", "損", "益", "慣", "罪", "検", "貿", "俵", "暴", "導", "価", "職", "証", "備", "婦", "評", "築", "支", "夢", "版"}},
			{"Sixth grade", {"宅", "幕", "針", "論", "割", "晩", "降", "済", "困", "染", "庁", "射", "届", "宙", "揮", "源", "秘", "誌", "操", "刻", "危", "映", "値", "異", "処", "暖", "洗", "縦", "展", "除", "供", "捨", "机", "訪", "詞", "窓", "郵", "私", "専", "頂", "認", "干", "背", "盛", "優", "存", "宇", "模", "密", "収", "若", "座", "賃", "泉", "呼", "痛", "段", "探", "枚", "閉", "簡", "忘", "誤", "看", "奏", "衆", "装", "絹", "翌", "蒸", "推", "陛", "穀", "胸", "俳", "視", "幼", "臓", "否", "潮", "樹", "厳", "班", "署", "冊", "磁", "砂", "善", "傷", "皇", "覧", "宣", "乳", "糖", "尊", "垂", "層", "勤", "株", "蔵", "敬", "担", "派", "延", "欲", "巻", "熟", "討", "誠", "純", "創", "蚕", "批", "脳", "憲", "宗", "従", "並", "系", "貴", "臨", "裁", "激", "后", "補", "穴", "縮", "障", "遺", "腹", "宝", "鋼", "朗", "孝", "沿", "肺", "棒", "忠", "革", "党", "亡", "納", "片", "仁", "奮", "律", "誕", "就", "将", "至", "紅", "訳", "権", "拡", "著", "我", "盟", "郷", "警", "灰", "吸", "裏", "筋", "尺", "拝", "難", "域", "己", "城", "寸", "疑", "策", "暮", "乱", "諸", "卵", "閣", "劇", "聖", "姿", "骨"}},
			{"Jōyō", {"範", "濃", "凡", "請", "廃", "含", "怒", "較", "菓", "被", "与", "皆", "恋", "環", "軸", "症", "威", "汁", "巾", "阪", "駐", "撮", "床", "排", "伴", "描", "途", "戻", "洪", "御", "扱", "超", "詳", "亭", "攻", "距", "丈", "雷", "唆", "呂", "冗", "影", "挿", "渇", "惑", "誰", "咲", "頼", "零", "換", "突", "占", "怖", "濯", "躍", "炉", "婚", "携", "依", "互", "扉", "遍", "昇", "袋", "到", "幅", "及", "滅", "替", "埋", "伯", "違", "添", "殊", "忙", "甘", "普", "析", "渡", "匂", "歳", "沢", "迎", "吹", "脱", "眠", "販", "渋", "辛", "彼", "払", "込", "棄", "寝", "枯", "湿", "更", "援", "悔", "僕", "絡", "響", "徴", "疲", "離", "謄", "循", "踊", "診", "弔", "柳", "弐", "嵐", "揺", "蚊", "填", "拓", "姓", "惰", "刑", "疎", "醸", "了", "窟", "沈", "懇", "諮", "噴", "緊", "稽", "菊", "旦", "渓", "充", "狙", "煩", "腺", "恭", "併", "鉛", "訃", "摯", "祉", "江", "租", "帽", "錦", "寿", "鎌", "避", "虐", "黙", "窮", "嫡", "縄", "恨", "臆", "隙", "礁", "妃", "藩", "拳", "鬼", "尻", "沙", "還", "藍", "賢", "妥", "聘", "稼", "騎", "滞", "盾", "杉", "妄", "繰", "汚", "肯", "戚", "隠", "獣", "崖", "頃", "燥", "瞭", "褒", "猛", "双", "劾", "悦", "儀", "陵", "繕", "敏", "丼", "擬", "崇", "拭", "遅", "庸", "彫", "阻", "叱", "廉", "摘", "魅", "況", "稚", "癖", "勧", "卓", "匠", "麓", "需", "浦", "鑑", "壊", "唯", "砲", "顎", "畔", "禍", "但", "彰", "仰", "香", "漬", "庶", "偏", "矛", "傾", "鯨", "棚", "患", "紹", "阜", "脂", "憾", "蹴", "堪", "睡", "爽", "佐", "緒", "厄", "叫", "遭", "祥", "雅", "巧", "津", "薫", "亜", "窃", "囚", "耗", "壮", "盆", "坑", "介", "秀", "斎", "暁", "逮", "舗", "屯", "匿", "飽", "遜", "謹", "弦", "踪", "闇", "宛", "宵", "坊", "麺", "逓", "遵", "覇", "芯", "狭", "択", "侍", "概", "吐", "潟", "輩", "撤", "尽", "湾", "聴", "抄", "輝", "崎", "掌", "霧", "裾", "戴", "維", "疫", "乙", "傍", "喝", "迅", "恥", "逸", "貌", "畜", "奨", "矯", "詔", "跳", "慄", "脚", "癒", "震", "譲", "又", "押", "憧", "即", "弊", "辱", "帆", "肪", "訴", "鍵", "岳", "殖", "巨", "寛", "泊", "尾", "慈", "弧", "尿", "珍", "紡", "仙", "鉢", "彩", "硝", "弄", "柄", "姫", "悼", "盤", "魂", "賦", "嘱", "抽", "牙", "殴", "審", "俺", "棟", "丙", "傲", "献", "堀", "艦", "暇", "斗", "譜", "遷", "契", "衝", "釈", "煙", "穏", "玄", "唄", "拍", "謀", "暫", "伺", "臭", "憎", "尚", "摂", "脅", "抱", "慮", "肝", "蛮", "郊", "擁", "壌", "炎", "宴", "膨", "伏", "柵", "斜", "茂", "幻", "搾", "礎", "鎮", "孔", "滝", "堕", "駆", "懸", "隻", "倣", "逃", "斥", "砕", "恵", "挫", "廷", "貼", "糾", "敷", "沃", "腰", "丘", "挨", "箋", "挑", "崩", "淡", "郎", "猫", "飢", "奇", "衷", "猶", "俗", "憬", "斤", "貢", "粘", "溶", "克", "稲", "菌", "慰", "釣", "寧", "尼", "膳", "痘", "耐", "蜜", "璃", "旋", "濁", "栓", "塔", "鈍", "濫", "媛", "獄", "裂", "曇", "惧", "畏", "啓", "塁", "抑", "駄", "欺", "是", "侶", "潤", "践", "骸", "尉", "託", "虚", "賓", "剰", "衡", "采", "鈴", "嫁", "貫", "磨", "蓄", "屈", "膚", "剖", "錠", "柔", "鼓", "寮", "溝", "催", "陰", "謎", "塞", "盲", "爪", "塗", "鍋", "振", "狩", "罷", "壁", "煮", "髄", "拒", "裕", "旺", "寂", "帥", "憚", "峡", "潜", "脊", "怠", "妖", "継", "闘", "呈", "培", "詣", "蔑", "娘", "哲", "殻", "抹", "括", "愚", "恣", "塀", "豚", "槽", "宜", "綱", "憂", "微", "凄", "咽", "塾", "粒", "索", "漂", "兼", "亀", "塊", "珠", "刺", "偽", "枕", "峠", "瑠", "碑", "慌", "却", "綻", "捗", "涙", "軒", "肌", "慶", "糧", "炊", "撃", "蛇", "悠", "監", "硬", "桟", "浸", "荘", "赦", "遇", "傑", "執", "涼", "浪", "勘", "朽", "累", "琴", "籍", "瓶", "償", "某", "如", "甲", "享", "棺", "慨", "邸", "紺", "襟", "謁", "妨", "妬", "淑", "羨", "翼", "縁", "韓", "卸", "盗", "免", "堤", "臼", "把", "溺", "閑", "遣", "酎", "錬", "肖", "旬", "繭", "隷", "薪", "瞳", "腕", "殿", "擦", "錯", "煎", "鶴", "貪", "麻", "漆", "冒", "握", "侮", "罰", "拙", "忍", "廊", "娯", "随", "岡", "秩", "髪", "凹", "雇", "虞", "褐", "猟", "爆", "餓", "厘", "渉", "諜", "幽", "欧", "膝", "触", "穫", "称", "履", "既", "箇", "汎", "荒", "揚", "賜", "端", "艇", "脇", "籠", "眉", "遂", "朱", "虹", "墨", "椅", "抜", "乏", "冥", "帝", "泌", "甚", "督", "匹", "凍", "沼", "壱", "鮮", "胎", "叔", "諾", "眺", "楼", "葬", "焦", "蛍", "肢", "碁", "誘", "拷", "剛", "辣", "漏", "掲", "峰", "幣", "爵", "儒", "銘", "漸", "削", "牲", "曖", "拘", "栽", "烈", "騒", "謙", "畳", "鍛", "粋", "欄", "顧", "誓", "陶", "唐", "沸", "奉", "繊", "賊", "薄", "涯", "胞", "桑", "詰", "墜", "靴", "滋", "趣", "那", "愁", "諧", "斬", "奔", "乞", "喫", "挟", "枠", "偵", "蔽", "澄", "紋", "傘", "痴", "捻", "拉", "汰", "昧", "惨", "股", "雰", "宰", "伐", "緩", "逐", "疾", "粛", "漠", "誇", "肩", "舶", "桁", "俊", "玩", "懐", "頒", "斉", "苛", "撲", "敢", "肘", "齢", "喪", "酔", "奈", "封", "伸", "箸", "僅", "斑", "冠", "促", "附", "洞", "泰", "窒", "哀", "刈", "紛", "戒", "致", "蓋", "紳", "剤", "惜", "跡", "銃", "隔", "奥", "紫", "搬", "劣", "冶", "懲", "昆", "慕", "串", "縛", "絞", "貞", "閲", "襲", "圏", "璧", "窯", "喩", "升", "羞", "諦", "酵", "餌", "迭", "召", "井", "没", "婿", "侵", "醒", "瞬", "勃", "梗", "麗", "吉", "姻", "氾", "顕", "頻", "閥", "虎", "浮", "戯", "郭", "嫌", "奴", "寡", "霊", "獲", "祈", "弥", "筒", "頑", "膜", "勲", "鹿", "竜", "掃", "旨", "憩", "丹", "据", "乾", "薦", "陣", "胴", "堅", "披", "核", "禅", "喉", "酷", "痕", "粗", "葛", "措", "踏", "茨", "療", "越", "嘲", "華", "賠", "滑", "鬱", "藻", "腫", "慢", "畿", "魔", "騰", "苗", "徐", "凝", "茎", "愉", "符", "項", "孤", "悩", "渦", "尋", "舷", "穂", "軌", "奪", "凸", "衰", "頓", "徹", "掘", "缶", "媒", "搭", "掛", "嚇", "墳", "邦", "壇", "熊", "淫", "透", "雄", "殉", "購", "諭", "犠", "緯", "忌", "霜", "朴", "舟", "瀬", "鎖", "侯", "桃", "嗣", "篤", "嗅", "誉", "架", "酢", "鐘", "載", "吟", "扶", "浜", "僚", "雌", "虜", "潰", "駒", "韻", "征", "扇", "逝", "婆", "捕", "醜", "拐", "胆", "吏", "須", "刃", "泥", "隣", "准", "枢", "妙", "佳", "杯", "瓦", "赴", "艶", "詠", "隅", "債", "偶", "恒", "鶏", "嘆", "豪", "怨", "湧", "歓", "坪", "妊", "餅", "邪", "憶", "贈", "簿", "俸", "裸", "巡", "僧", "控", "憤", "嬢", "呉", "勅", "覆", "迫", "舞", "芋", "款", "硫", "酬", "拶", "浄", "倹", "繁", "痩", "滴", "畝", "賄", "訂", "瘍", "曾", "頬", "般", "幾", "詮", "岬", "露", "剥", "曹", "陪", "袖", "捜", "垣", "施", "猿", "倒", "腎", "朕", "稿", "恐", "慎", "轄", "芝", "羅", "酌", "鋳", "唾", "締", "痢", "塑", "励", "墾", "剣", "陳", "罵", "叙", "埼", "萎", "抵", "弾", "抗", "縫", "詐", "捉", "怪", "狂", "飾", "娠", "為", "晶", "漫", "卑", "企", "謡", "嫉", "翻", "呪", "栃", "泡", "梨", "暦", "且", "遮", "緻", "軟", "摩", "彙", "沖", "拠", "驚", "粧", "網", "悟", "蜂", "偉", "翁", "凶", "鋭", "塚", "藤", "腐", "唇", "堆", "伎", "酪", "隆", "訟", "汗", "棋", "芳", "融", "倫", "喚", "璽", "陥", "岐", "募", "房", "該"}},
			{"Jinmeiyō", {"伊", "苺", "哨", "茜", "蒲", "皐", "沓", "徽", "鳶", "螺", "洛", "燕", "鞍", "梯", "鵬", "颯", "蟬", "燿", "瑛", "彬", "葵", "辿", "紬", "楠", "瓢", "沫", "杭", "捺", "俄", "浩", "渥", "輔", "杷", "袴", "箕", "欣", "朔", "閏", "綸", "芥", "漕", "鯉", "函", "麟", "楕", "堰", "肇", "迪", "雛", "昊", "殆", "翔", "槙", "纏", "諒", "鞘", "稀", "樟", "偲", "頗", "襖", "惟", "撫", "萌", "玖", "瑳", "蒔", "鷗", "兎", "嵩", "綜", "竪", "笙", "幌", "蔓", "沌", "嘗", "溢", "杵", "焚", "椀", "蕨", "薩", "祇", "倭", "霞", "佑", "梓", "祢", "焰", "逞", "蕪", "鳩", "掠", "椛", "穹", "蕃", "伽", "鼎", "秦", "荻", "叉", "栗", "槌", "芦", "舜", "註", "嘩", "頌", "朋", "喬", "凰", "巌", "滉", "於", "逗", "萊", "琳", "頁", "磯", "此", "絢", "塙", "捲", "筈", "摺", "舵", "喋", "之", "勁", "莫", "豹", "匡", "詢", "宕", "洸", "珂", "檀", "跨", "歎", "蘇", "壕", "禄", "郁", "苔", "董", "菅", "蝦", "笠", "梧", "姥", "煉", "蹄", "淳", "輯", "諄", "柾", "讃", "稔", "奄", "萩", "彦", "薙", "馨", "禎", "櫛", "燦", "聡", "恢", "楢", "纂", "弛", "獅", "狼", "瑞", "芹", "蕉", "莞", "瑚", "琢", "鷺", "閤", "嬉", "峻", "嶺", "訊", "鳳", "昏", "帖", "巷", "錘", "筑", "驍", "菖", "釧", "蔦", "耶", "膏", "湊", "托", "絆", "些", "姪", "衿", "稟", "勺", "洲", "酉", "珀", "迂", "杖", "脩", "胤", "叶", "尭", "亦", "疋", "晒", "茄", "按", "鱒", "槍", "祐", "侑", "晦", "斐", "轟", "萄", "劫", "杜", "儲", "葡", "勿", "瀕", "湘", "稜", "糊", "云", "楚", "鞠", "裳", "俐", "鯛", "伍", "斧", "嵯", "澪", "挺", "吾", "彗", "淵", "茅", "櫂", "笈", "哉", "毬", "擢", "竣", "隼", "窄", "惹", "柏", "祁", "烏", "暢", "碩", "斯", "疏", "馴", "矩", "寵", "廻", "汲", "諏", "允", "廟", "樺", "李", "鷲", "毘", "捧", "遼", "牒", "閃", "誼", "叢", "楊", "釉", "槻", "夷", "靖", "漱", "晋", "瑶", "鮎", "竿", "玲", "栞", "啄", "辻", "娃", "弘", "熙", "鴨", "菫", "麒", "樋", "曙", "栖", "梁", "伶", "魁", "鰯", "毅", "萱", "魯", "灼", "蹟", "汐", "鱗", "鎧", "耽", "椰", "簾", "蒙", "劉", "蘭", "窪", "鄭", "傭", "已", "俣", "椿", "棲", "淀", "釘", "而", "厨", "旭", "寓", "磐", "灘", "悉", "芭", "茉", "絃", "巳", "晏", "篠", "曳", "榛", "乎", "檎", "琵", "燎", "亨", "碗", "桶", "瞥", "揃", "禽", "牽", "莉", "浬", "肴", "乃", "辰", "智", "緋", "蔭", "蟹", "裡", "煌", "恕", "雫", "惺", "兜", "醇", "凌", "坦", "梢", "杏", "迄", "邑", "碓", "掬", "櫓", "縞", "醬", "溜", "叡", "也", "胡", "孟", "迦", "犀", "琥", "惚", "戟", "庄", "凱", "葦", "牟", "醐", "鋒", "錫", "撒", "蔣", "輿", "戊", "惇", "只", "淋", "枇", "恰", "亘", "廿", "祷", "諺", "庚", "臥", "琉", "馳", "蝶", "碧", "蕎", "蒼", "汝", "皓", "翠", "喰", "綴", "菱", "橘", "尤", "亮", "蓬", "麿", "孜", "鞄", "柑", "播", "曝", "禾", "紗", "丑", "灸", "屑", "畢", "凧", "錆", "堵", "蕗", "晟", "茸", "黛", "椋", "隈", "亥", "倦", "鋸", "圃", "濡", "脹", "俠", "奎", "赳", "噂", "蓮", "凜", "庇", "凪", "尖", "詫", "雀", "遥", "俱", "庵", "砥", "柊", "腔", "昴", "阿", "鞭", "喧", "菩", "裟", "欽", "葺", "醍", "坐", "鍬", "堺", "宋", "楯", "侃", "謂", "峨", "燭", "穣", "忽", "湛", "宥", "卯", "怜", "蓑", "猪", "漣", "幡", "苑", "慧", "牡", "仔", "這", "賑", "捷", "甫", "蕾", "崚", "敦", "圭", "娩", "埴", "或", "陀", "琶", "竺", "昂", "梛", "蠟", "楓", "爾", "芙", "珈", "桔", "黎", "穿", "悌", "鵜", "徠", "綺", "桐", "洵", "鷹", "粟", "厩", "暉", "渚", "藁", "籾", "巽", "硯", "紐", "昌", "耀", "綾", "斡", "簞", "繡", "蓉", "駕", "卜", "噌", "羚", "橙", "榊", "彪", "銑", "樽", "晨", "榎", "珊", "晃", "駿", "眸", "煤", "惣", "丞", "佃", "憐", "撰", "宏", "哩", "樫", "柘", "柴", "梶", "冴", "寅", "匁", "粥", "其", "瓜", "肋", "笹", "卿", "袈", "挽", "窺", "遁", "雁", "繫", "壬", "吞", "錐", "撞", "砦", "紘", "畠", "逢", "桂", "蒐", "秤", "砧", "貰", "巴", "箔", "嘉", "吻", "訣", "甥", "倖", "桧", "柚", "汀", "套", "篇", "顚", "饗", "鴻"}}
		};
		
		checksql(sqlite3_open(fname.c_str(), &db), "Couldn't create deck database");
		transac_begin();
		sqlite3_stmt *stmt;
		for (const std::string &create : schema)
		{
			checksql(sqlite3_prepare_v2(db, create.c_str(), -1, &stmt, nullptr), "Failed to set up database schema");
			checksql(sqlite3_step(stmt), "Failed to set up database schema");
			sqlite3_finalize(stmt);
		}
		for (const std::string &field : fieldnames)
		{
			checksql(sqlite3_prepare_v2(db, "insert into `fieldname` (`name`) values (?)", -1, &stmt, nullptr), "Failed to set up fields in database");
			checksql(sqlite3_bind_text(stmt, 1, field.c_str(), -1, nullptr), "Failed to bind field name");
			checksql(sqlite3_step(stmt), "Failed to set up fields in database");
			sqlite3_finalize(stmt);
		}
		checksql(sqlite3_prepare_v2(db, "insert into `info` (`version`, `step`, `laststep`) values (?, ?, ?)", -1, &stmt, nullptr), "Failed to set up database info");
		checksql(sqlite3_bind_int(stmt, 1, db_version));
		checksql(sqlite3_bind_int(stmt, 2, 0));
		checksql(sqlite3_bind_int(stmt, 3, midnight()));
		checksql(sqlite3_step(stmt), "Failed to set up database info");
		sqlite3_finalize(stmt);
		checksql(sqlite3_prepare_v2(db, "insert into `prefs` (`autostep`) values (?)", -1, &stmt, nullptr), "Failed to set up database preferences");
		checksql(sqlite3_bind_int(stmt, 1, 0));
		checksql(sqlite3_step(stmt), "Failed to set up database preferences");
		sqlite3_finalize(stmt);
		transac_end();
		sqlite3_close(db);
		db = nullptr;
	}
	
	void populate()
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		// TODO Check if DB is locked for editing
		
		checksql(sqlite3_prepare_v2(db, "select `name` from `deck`", -1, &stmt, nullptr), "Failed to fetch deck names");
		while (sqlite3_step(stmt) == SQLITE_ROW) Deck::add(std::string{(const char *) sqlite3_column_text(stmt, 0)});
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `id`, `deck`, `step`, `interval`, `status`, `upd_norm`, `upd_decr`, `upd_incr`, `upd_reset` from `card`", -1, &stmt, nullptr), "Failed to fetch cards");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			int id{sqlite3_column_int(stmt, 0)};
			std::string deckname{(const char *) sqlite3_column_text(stmt, 1)};
			int step{sqlite3_column_int(stmt, 2)};
			int interval{sqlite3_column_int(stmt, 3)};
			Card::Status status{(Card::Status) sqlite3_column_int(stmt, 4)};
			std::unordered_map<Card::UpdateType, int, Card::uthash> count{{Card::UpdateType::NORM, sqlite3_column_int(stmt, 5)}, {Card::UpdateType::DECR, sqlite3_column_int(stmt, 6)}, {Card::UpdateType::INCR, sqlite3_column_int(stmt, 7)}, {Card::UpdateType::RESET, sqlite3_column_int(stmt, 8)}};
			std::unordered_map<std::string, std::string> fields{};
			sqlite3_stmt *stmt2;
			checksql(sqlite3_prepare_v2(db, "select `field`, `value` from `field` where `card` = ?", -1, &stmt2, nullptr), "Failed to fetch fields");
			checksql(sqlite3_bind_int(stmt2, 1, id), "Failed to bind deck name");
			while (sqlite3_step(stmt2) == SQLITE_ROW)
			{
				std::string field{(const char *) sqlite3_column_text(stmt2, 0)};
				std::string value{(const char *) sqlite3_column_text(stmt2, 1)};
				fields[field] = value;
			}
			sqlite3_finalize(stmt2);
			Card::add(Deck::get(deckname), id, fields, step, interval, count, status, 0, true);
		}
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `deck`, `name` from `character_category`", -1, &stmt, nullptr), "Failed to set up character categories"); // Using this for ordering is probably a bad idea because I don't think SQLite guarantees consistent ordering of its rows.
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string deckname{(const char *) sqlite3_column_text(stmt, 0)};
			std::string secname{(const char *) sqlite3_column_text(stmt, 1)};
			Deck::get(deckname).bank().addsect(secname);
		}
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `deck`, `category`, `character`, `active`, `step`, `count` from `character`", -1, &stmt, nullptr), "Failed to fetch characters");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string deckname{(const char *) sqlite3_column_text(stmt, 0)};
			std::string category{(const char *) sqlite3_column_text(stmt, 1)};
			std::string character{(const char *) sqlite3_column_text(stmt, 2)};
			bool active{(bool) sqlite3_column_int(stmt, 3)};
			int step{sqlite3_column_int(stmt, 4)};
			int count{sqlite3_column_int(stmt, 5)};
			Bank &b = Deck::get(deckname).bank();
			b.add(category, character);
			if (active) b.enable(character, step, count, true);
		}
		sqlite3_finalize(stmt);
		//for (const Card &card : Card::cards()) for (std::string field : Card::fieldnames()) if (! card.hasfield(field)) throw std::runtime_error{"Card " + util::t2s(card.id()) + " missing field " + field};
		Deck::rebuild_all();
	}
	
	void early_populate()
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "select `version`, `step`, `laststep` from `info`", -1, &stmt, nullptr), "Failed to verify database version");
		if (sqlite3_step(stmt) != SQLITE_ROW) throw std::runtime_error{"Failed to verify database version"};
		int ver = sqlite3_column_int(stmt, 0);
		if (ver != db_version) throw std::runtime_error{"Program requires database of version " + util::t2s(db_version) + ", but current database is version " + util::t2s(ver)};
		Deck::curstep = sqlite3_column_int(stmt, 1);
		std::time_t laststep = sqlite3_column_int(stmt, 2);
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "select `autostep` from `prefs`", -1, &stmt, nullptr), "Failed to retrieve preferences");
		if (sqlite3_step(stmt) != SQLITE_ROW) throw std::runtime_error{"Failed to retrieve preferences"};
		bool auto_step = sqlite3_column_int(stmt, 0);
		sqlite3_finalize(stmt);
		
		int diff = (midnight() - laststep) / (24 * 3600); // No leap seconds
		if (auto_step && diff > 0)
		{
			std::cout << "Advancing decks by " << diff << " steps\n";
			Deck::curstep += diff;
			step(diff);
		}
		
		checksql(sqlite3_prepare_v2(db, "select `name` from `fieldname`", -1, &stmt, nullptr), "Failed to fetch field names");
		while (sqlite3_step(stmt) == SQLITE_ROW)
		{
			std::string fieldname{(const char *) sqlite3_column_text(stmt, 0)};
			Card::fieldnames_.push_back(fieldname);
			Card::deffields_[fieldname] = "";
		}
		sqlite3_finalize(stmt);
	}
	
	void commit()
	{
		//sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};	
	}
	
	void transac_begin()
	{
		checksql(sqlite3_exec(db, "begin", 0, 0, 0));
	}
	
	void transac_end()
	{
		checksql(sqlite3_exec(db, "commit", 0, 0, 0));
	}
	
	void card_update(const Card &card)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
	
		bool newcard = false;
		checksql(sqlite3_prepare_v2(db, "select * from `card` where `id` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		if (sqlite3_step(stmt) != SQLITE_ROW) newcard = true;
		sqlite3_finalize(stmt);
		
		if (newcard) checksql(sqlite3_prepare_v2(db, "insert into `card` (`deck`, `step`, `interval`, `status`, `upd_norm`, `upd_decr`, `upd_incr`, `upd_reset`, `id`) values (?, ?, ?, ?, ?, ?, ?, ?, ?)", -1, &stmt, nullptr));
		else checksql(sqlite3_prepare_v2(db, "update `card` set `deck` = ?, `step` = ?, `interval` = ?, `status` = ?, `upd_norm` = ?, `upd_decr` = ?, `upd_incr` = ?, `upd_reset` = ? where `id` = ?", -1, &stmt, nullptr));
		
		std::string name = card.deck()->canonical();
		checksql(sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr));
		checksql(sqlite3_bind_int(stmt, 2, card.step()));
		checksql(sqlite3_bind_int(stmt, 3, card.delay()));
		checksql(sqlite3_bind_int(stmt, 4, (int) card.status()));
		checksql(sqlite3_bind_int(stmt, 5, card.count()[Card::UpdateType::NORM]));
		checksql(sqlite3_bind_int(stmt, 6, card.count()[Card::UpdateType::DECR]));
		checksql(sqlite3_bind_int(stmt, 7, card.count()[Card::UpdateType::INCR]));
		checksql(sqlite3_bind_int(stmt, 8, card.count()[Card::UpdateType::RESET]));
		checksql(sqlite3_bind_int(stmt, 9, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void card_edit(const Card &card, const std::string &field)
	{
		std::string value = card.field(field); // The fact that this is necessary may indicate deeper issues with memory management
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `field` where `card` = ? and `field` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_bind_text(stmt, 2, field.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "insert into `field` (`card`, `field`, `value`) values (?, ?, ?)", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_bind_text(stmt, 2, field.c_str(), -1, nullptr));
		checksql(sqlite3_bind_text(stmt, 3, value.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void card_del(const Card &card)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `card` where `id` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
		
		checksql(sqlite3_prepare_v2(db, "delete from `field` where `card` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, card.id()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void deck_edit(const Deck &deck, std::string oldname)
	{
		if (! deck.explic()) return;
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		std::string name = (oldname != "" ? oldname : deck.canonical());
		bool newdeck = false;
		checksql(sqlite3_prepare_v2(db, "select * from deck where `name` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, 1, name.c_str(), -1, nullptr));
		if (sqlite3_step(stmt) != SQLITE_ROW) newdeck = true;
		sqlite3_finalize(stmt);
		
		if (newdeck) checksql(sqlite3_prepare_v2(db, "insert into `deck` (`name`) values (?)", -1, &stmt, nullptr));
		else
		{
			checksql(sqlite3_prepare_v2(db, "update `deck` set `name` = ? where `name` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, name.c_str(), -1, nullptr));
		}
		checksql(sqlite3_bind_text(stmt, 1, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void deck_del(const Deck &deck)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "delete from `deck` where `name` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, 1, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void bank_edit(const Deck &deck, const std::string &character, bool active, int step, int count, std::string olddeck)
	{
		//std::cout << "Update " << character << " in deck " << deck.canonical() << " from deck " << olddeck << " to active = " << active << ", step = " << step << ", count = " << count << "\n";
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		if (olddeck != "" && olddeck != deck.canonical())
		{
			std::string newname{deck.canonical()};
			
			checksql(sqlite3_prepare_v2(db, "update `character` set `deck` = ? where `deck` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 1, newname.c_str(), -1, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, olddeck.c_str(), -1, nullptr));
			checksql(sqlite3_step(stmt));
			sqlite3_finalize(stmt);

			checksql(sqlite3_prepare_v2(db, "update `character_category` set `deck` = ? where `deck` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_text(stmt, 1, newname.c_str(), -1, nullptr));
			checksql(sqlite3_bind_text(stmt, 2, olddeck.c_str(), -1, nullptr));
			checksql(sqlite3_step(stmt));
			sqlite3_finalize(stmt);
		}
		
		int i = 1;
		if (active)
		{
			checksql(sqlite3_prepare_v2(db, "update `character` set `active` = '1', `step` = ?, `count` = ? where `deck` = ? and `character` = ?", -1, &stmt, nullptr));
			checksql(sqlite3_bind_int(stmt, i++, step));
			checksql(sqlite3_bind_int(stmt, i++, count));
		}
		else checksql(sqlite3_prepare_v2(db, "update `character` set `active` = '0' where `deck` = ? and `character` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_text(stmt, i++, deck.canonical().c_str(), -1, nullptr));
		checksql(sqlite3_bind_text(stmt, i++, character.c_str(), -1, nullptr));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
	
	void step(int offset)
	{
		sqlite3_stmt *stmt;
		if (db == nullptr) throw std::runtime_error{"Database connection unexpectedly closed"};
		
		checksql(sqlite3_prepare_v2(db, "update `info` set `step` = `step` + ?, `laststep` = ?", -1, &stmt, nullptr));
		checksql(sqlite3_bind_int(stmt, 1, offset));
		checksql(sqlite3_bind_int(stmt, 2, midnight()));
		checksql(sqlite3_step(stmt));
		sqlite3_finalize(stmt);
	}
}
