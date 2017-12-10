/* 
 * File:   Bank.cpp
 * Author: matt
 * 
 * Created on December 2, 2013, 5:02 PM
 */

#include "Bank.h"
#include "Deck.h"

//std::unordered_map<std::string, std::unordered_set<std::string>> Bank::basis{};

/*Bank &Bank::operator =(const Bank& orig)
{
	if (this == &orig) return *this;
	words_ = orig.words_;
	inset_ = orig.inset_;
	field_ = orig.field_;
	deck_ = orig.deck_;
	basis_ = orig.basis_;
	return *this;
}*/

std::vector<std::string> Bank::tokenize(std::string str)
{
	std::vector<std::string> ret{};
	while (str != "")
	{
		std::string ch = util::utf8_popchar(str);
		int64_t c = util::utf8_codepoint(ch);
		if ((c >= 0x4E00 && c < 0xA000) || (c >= 0x3400 && c < 0x4DC0) || (c >= 0x20000 && c < 0x2A6E0) || (c >= 0x2A700 && c < 0x2B740) || (c > 0x2B740 && c < 0x2B820) || (c > 0xF900 && c < 0xFB00) || (c > 0x2E80 && c < 0x2FE0) || (c > 0x3000 && c < 0x3040) || (c > 0xF900 && c < 0xFB00) || (c > 0x2F800 && c < 0x2FA20) || (c > 0xFE30 && c < 0xFE50))
			ret.push_back(ch);
	}
	return ret;
}

int Bank::BankItem::offset() const
{
	return step - Deck::curstep;
}

Bank &Bank::inherited() const
{
	return deck_->inherited()->bank();
}

Bank::Section &Bank::sect(std::string name)
{
	if (! deck_->explic()) return inherited().sect(name);
	std::list<Section>::iterator s = std::find_if(basis_.begin(), basis_.end(), [&name](const Section &section) { return section.name == name; });
	if (s != basis_.end()) return *s;
	basis_.push_back(Section{name});
	return basis_.back();
}

bool Bank::check(Card *card) // True if the card should go in kanji, preventing multiple cards with the same kanji from being placed in the same set // <-- This doesn't work because duplicate kanji are detected at the leaf decks, and higher-level decks are the sum of the child decks.  Disabled.
{
	if (! words().size()) return false;
	const std::vector<std::string> cardwords = tokenize(card->field(field_));
	if (! cardwords.size()) return false;
	//for (const std::string &word : cardwords) if (! words().count(word) || inset_.count(word) || words().at(word).offset() > 0) return false;
	for (const std::string &word : cardwords) if (! words().count(word) || words().at(word).offset() > 0) return false;
	for (const std::string &word : cardwords) inset_.insert(word);
	return true;
}

bool Bank::enable(std::string word, int step, unsigned int n, bool fromdb)
{
	if (step == -1) step = Deck::curstep;
	if (words().count(word)) return false;
	words().insert(std::make_pair(word, BankItem(step, n)));
	
	if (! fromdb) backend::bank_edit(*(inherited().deck_), word, 1, step, n);
	return true;
}

bool Bank::disable(std::string word)
{
	if (! words().count(word)) return false;
	words().erase(word);
	backend::bank_edit(*(inherited().deck_), word, 0, 0, 0);
	return true;
}

bool Bank::enabled(std::string word) const
{
	return words().count(word) > 0;
}

void Bank::update(Card *card, Card::UpdateType type) // TODO offset
{
	if (! card->due(0)) for (const std::string &word : tokenize(card->field(field_)))
	{
		if (words().count(word))
		{
			BankItem &bi = words().at(word);
			bi.step = Deck::curstep + 1;
			bi.practices++;
			// assert(inset_.count(word));
			if (inset_.count(word)) inset_.erase(inset_.find(word));
			backend::bank_edit(*(inherited().deck_), word, 1, bi.step, bi.practices);
		}
	}
}

void Bank::commit(std::string olddeck) const
{
	backend::transac_begin();
	for (const Section &s : inherited().basis_) for (const std::string &word : s.words)
	{
		if (words().count(word))
		{
			const BankItem &bi = words().at(word);
			backend::bank_edit(*(inherited().deck_), word, 1, bi.step, bi.practices, olddeck);
		}
		else backend::bank_edit(*(inherited().deck_), word, 0, 0, 0, olddeck);
	}
	backend::transac_end();
}

void Bank::shift(int diff)
{
	for (std::pair<const std::string, BankItem> &word : words())
	{
		word.second.step += diff;
		backend::bank_edit(*(inherited().deck_), word.first, 1, word.second.step, word.second.practices);
	}
}

std::vector<std::string> Bank::vectorize(std::string word, const std::vector<coldesc> &colspec) const
{
	std::vector<std::string> ret{};
	for (coldesc col : colspec)
	{
		if (col.title == "Deck") ret.push_back(inherited().deck_->canonical());
		else if (col.title == "Word") ret.push_back(word);
		else if (col.title == "Offset") ret.push_back(util::t2s(words().at(word).offset()));
		else if (col.title == "Practices") ret.push_back(util::t2s<int>(words().at(word).practices));
		else ret.push_back("NULL");
	}
	return ret;
}

/*std::vector<std::string> Bank::wordlist() const
{
	std::vector<std::string> ret{};
	for (const std::pair<std::string, BankItem> word : words()) ret.push_back(word.first);
	return ret;
}*/

std::string Bank::htmlview() const
{
	std::stringstream ret{};
	ret << "<html><body>";
	for (const Section &section : inherited().basis_)
	{
		ret << "<h3>" << section.name << "</h3>";
		for (const std::string &word : section.words) ret << "<bankbtn name=\"" << word << "\">"; // "<font color=" << (words_.count(word) ? "black" : "gray") << "><a href=\"" << word << "\">" << word << "</a></font>&nbsp;"
	}
	ret << "</html></body>";
	return ret.str();
}
