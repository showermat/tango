/* 
 * File:   Card.cpp
 * Author: matt
 * 
 * Created on July 27, 2013, 6:20 PM
 */

#include "Card.h"
#include "Deck.h"
#include <iostream>

std::vector<std::string> Card::fieldnames_{};
std::unordered_map<std::string, std::string> Card::deffields_{};
const double Card::ratio_ = 2;
const int Card::maxdelay_ = 365;
std::list<Card> Card::cards_{};
int Card::cardnum_ = 1;

std::string Card::stat2str(Status status)
{
	if (status == Status::OK) return "Active";
	if (status == Status::DONE) return "Done";
	if (status == Status::SUSP) return "Suspended";
	if (status == Status::LEECH) return "Leech";
	throw std::runtime_error{"Impossible error in stat2str"};
}

Card::Status Card::str2stat(std::string status)
{
	if (status == "Active") return Status::OK;
	if (status == "Done") return Status::DONE;
	if (status == "Suspended") return Status::SUSP;
	if (status == "Leech") return Status::LEECH;
	throw std::runtime_error{"Invalid status string \"" + status + "\" passed to str2stat"};
}

std::string Card::sit2str(Field sit)
{
	if (sit == Field::NONE) return "None";
	if (sit == Field::KANJI) return "Kanji";
	if (sit == Field::HIRAGANA) return "Hiragana";
	if (sit == Field::FURIGANA) return "Furigana";
	if (sit == Field::MEANING) return "Meaning";
	throw std::runtime_error{"Impossible error in sit2str"};
}

std::string Card::sits2str(std::vector<Card::Field> sits)
{
	std::string ret{};
	if (! sits.size()) return ret;
	ret = sit2str(sits[0]);
	for (unsigned int i = 1; i < sits.size(); i++) ret += std::string{","} + sit2str(sits[i]);
	return ret;
}

Card::Field Card::str2sit(std::string str)
{
	if (str == "None") return Field::NONE;
	if (str == "Kanji") return Field::KANJI;
	if (str == "Hiragana") return Field::HIRAGANA;
	if (str == "Furigana") return Field::FURIGANA;
	if (str == "Meaning") return Field::MEANING;
	//if (str == "All") return Field::ALL;
	throw std::runtime_error{"Invalid set item type string \"" + str + "\" passed to str2sit"};
}

std::string Card::html_furigana(std::string kanji, std::string furigana)
{
	int kanjisize = 8;
	int kanasize = 3;
	//std::string kanji{kanji_s}, furigana{furigana_s};
	std::stringstream ret{};
	ret << "<table cellpadding=0><tr>";
	while (kanji != "")
	{
		std::string kchar = util::utf8_popchar(kanji);
		std::string kfur = util::strto(furigana, ",");
		while (kfur.length() > 0 && kfur.at(0) == '.')
		{
			kchar += util::utf8_popchar(kanji);
			kfur = kfur.substr(1);
		}
		if (kfur == "") kfur = "&nbsp;";
		ret << "<td valign=bottom><center><font size=" << kanasize << ">" << kfur << "</font><br><font size=" << kanjisize << ">" << kchar << "</font></center></td>";
	}
	ret << "</tr></table>";
	return ret.str();
}

std::string Card::hiragana(std::string kanji, std::string furigana)
{
	//std::string kanji{kanji_s}, furigana{furigana_s};
	std::stringstream ret{};
	while (kanji != "")
	{
		std::string kchar = util::utf8_popchar(kanji);
		std::string kfur = util::strto(furigana, ",");
		while (kfur.length() > 0 && kfur.at(0) == '.')
		{
			kchar += util::utf8_popchar(kanji);
			kfur = kfur.substr(1);
		}
		if (kfur == "") ret << kchar;
		else ret << kfur;
	}
	return ret.str();
}

Card &Card::add(Deck &deck, int id, std::unordered_map<std::string, std::string> fieldlist, int offset, int delay, std::unordered_map<UpdateType, int, uthash> count, Card::Status status, int statinfo, bool fromdb)
{
	int step;
	if (fromdb) step = offset;
	else step = offset + Deck::curstep;
	cards_.emplace_back(Card{id, &deck, fieldlist, step, delay, count, status, statinfo});
	cardnum_ = std::max(id, cardnum_);
	Card &c = cards_.back();
	deck.addcard(c, ! fromdb);
	if (! fromdb) backend::card_update(c);
	if (! fromdb) for (const std::string &field : Card::fieldnames()) backend::card_edit(c, field);
	return c;
}

Card &Card::create(Deck &deck)
{
	return add(deck, ++cardnum_, {}, 0, 0, {}, Card::Status::OK, 0);
}

void Card::del(Card &card, bool refresh, bool explic)
{
	Deck *deck = card.deck_;
	deck->delcard(card, refresh);
	card.deck_ = nullptr; // Prevent infinite loops of deletion!
	for (Deck *cur = deck; cur != &Deck::root; cur = cur->parent())
	{
		if (! cur->explic() && ! cur->size() && ! cur->children().size() && ! explic) Deck::del(*cur);
		else break;
	}
	if (backend::db && refresh) backend::card_del(card);
	cards_.erase(std::find(cards_.begin(), cards_.end(), card));
}

void Card::edit(Deck &deck, int offset, int delay, Status status)
{
	if (&deck != deck_)
	{
		Deck *olddeck = deck_;
		assert(olddeck->size() > 0);
		olddeck->delcard(*this);
		deck_ = nullptr;
		deck.addcard(*this);
		deck_ = &deck;	
	}
	step_ = Deck::curstep + offset;
	delay_ = delay;
	status_ = status;
	deck_->build();
	backend::card_update(*this);
}

std::vector<std::string> Card::vectorize(const std::vector<coldesc> &colspec) const
{
	std::vector<std::string> ret{};
	for (coldesc col : colspec)
	{
		if (col.title == "Deck") ret.push_back(deck_->canonical());
		else if (col.title == "Status") ret.push_back(stat2str(status_));
		else if (col.title == "Offset") ret.push_back(util::t2s(offset()));
		else if (col.title == "Interval") ret.push_back(util::t2s<int>(delay_));
		else if (col.title == "Norm") ret.push_back(util::t2s<int>(count_.at(UpdateType::NORM)));
		else if (col.title == "Incr") ret.push_back(util::t2s<int>(count_.at(UpdateType::INCR)));
		else if (col.title == "Decr") ret.push_back(util::t2s<int>(count_.at(UpdateType::DECR)));
		else if (col.title == "Reset") ret.push_back(util::t2s<int>(count_.at(UpdateType::RESET)));
		else if (std::find(std::begin(fieldnames_), std::end(fieldnames_), col.title) != fieldnames_.end()) ret.push_back(fields_.at(col.title));
		else ret.push_back("NULL");
	}
	return ret;
}

std::string Card::display(std::vector<Field> fields) const
{
	int kanasize;
	std::stringstream ret{};
	for (Field field : fields)
	{
		switch (field)
		{
			case Field::KANJI:
				ret << html_furigana(fields_.at("Expression"), "");
				break;
			case Field::HIRAGANA:
				kanasize = (fields.size() == 1) ? 8 : 4;
				ret << "<font size=" << kanasize << ">";
				if (fields_.at("Reading") == "") ret << fields_.at("Expression");
				else ret << hiragana(fields_.at("Expression"), fields_.at("Reading"));
				ret << "</font>";
				break;
			case Field::FURIGANA:
				ret << html_furigana(fields_.at("Expression"), fields_.at("Reading"));
				break;
			case Field::MEANING:
				ret << "<font size=4><b>" + fields_.at("Meaning") + "</b></font>";
				break;
			//case Field::ALL: return html_furigana(fields_.at("Expression"), fields_.at("Reading")) + "<br><br><font size=4><b>" + fields_.at("Meaning") + "</b></font>";
			case Field::NONE: ret << "";
		}
		ret << "<br><br>";
	}
	return ret.str();
}

bool Card::avail() const
{
	if (status_ == Status::DONE || status_ == Status::LEECH || status_ == Status::SUSP) return false;
	return true;
}

bool Card::due(int diff) const
{
	if (! avail()) return false;
	if (offset() <= diff) return true;
	return false;
}

bool Card::match(std::string query) const
{
	if (query == "") return true;
	for (const std::pair<std::string, std::string> &field : fields_) if (field.second.find(query) != std::string::npos) return true;
	if (hiragana(fields_.at("Expression"), fields_.at("Reading")).find(query) != std::string::npos) return true;
	return false;
}

void Card::shift(int diff)
{
	if (status_ == Status::SUSP) return;
	step_ += diff;
	backend::card_update(*this);
}

void Card::update(UpdateType type)
{
	// TODO Leeching (or eliminate -- will require adding a field)
	switch (type)
	{
		case UpdateType::NONE:
		case UpdateType::BURY:
			break;
		case UpdateType::NORM:
			step_ = Deck::curstep + delay_;
			break;
		case UpdateType::INCR:
			if (delay_ == 0) delay_ = 1;
			else delay_ *= ratio_;
			step_ = Deck::curstep + delay_;
			if (delay_ > maxdelay_) status_ = Status::DONE;
			break;
		case UpdateType::DECR:
			delay_ /= ratio_;
			step_ = Deck::curstep + delay_;
			break;
		case UpdateType::RESET:
			status_ = Status::OK;
			delay_ = 0;
			step_ = Deck::curstep;
			break;
		case UpdateType::DONE:
			status_ = Status::DONE;
			break;
		case UpdateType::SUSP:
			status_ = Status::SUSP;
			break;
		case UpdateType::LEECH:
			status_ = Status::LEECH;
			break;
		case UpdateType::RESUME:
			status_ = Status::OK;
			break;
	}
	count_[type]++;
	backend::card_update(*this);
}

int Card::offset() const
{
	return step_ - Deck::curstep;
}
