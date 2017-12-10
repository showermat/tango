/* 
 * File:   Card.h
 * Author: matt
 *
 * Created on July 27, 2013, 6:20 PM
 */

#ifndef CARD_H
#define	CARD_H

#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <algorithm>
#include <stdexcept>
#include <iostream> // TODO Debug remove
#include <cassert>
#include "util.h"
#include "coldesc.h"
#include "backend.h"

class Deck;

class Card
{
private:
	friend void backend::early_populate(); // TODO Replace with getters and setters
	friend void backend::populate(); // TODO Same
	static std::vector<std::string> fieldnames_;
	static std::unordered_map<std::string, std::string> deffields_;
	static std::unordered_map<std::string, std::string> fieldpref_, fieldsuff_;	
	static std::list<Card> cards_;
	static const double ratio_;
	static const int maxdelay_;
	static int cardnum_;
public:
	enum class Status { OK = 1, SUSP = 2, DONE = 3, LEECH = 4 };
	enum class UpdateType { NONE, INCR, DECR, NORM, RESET, SUSP, LEECH, BURY, RESUME, DONE };
	enum class Field { NONE, KANJI, HIRAGANA, FURIGANA, MEANING };
	struct uthash { size_t operator ()(const UpdateType &x) const { return static_cast<size_t>(x); } };

	static std::vector<std::string> fieldnames() { return fieldnames_; }
	static int maxdelay() { return maxdelay_; }
	static std::string stat2str(Status status);
	static Status str2stat(std::string status);
	static std::string sit2str(Field sit);
	static std::string sits2str(std::vector<Field> sits);
	static Field str2sit(std::string str);
	static std::string html_furigana(std::string kanji, std::string furigana);
	static std::string hiragana(std::string kanji, std::string furigana);
	static Card &add(Deck &deck, int id = ++cardnum_, std::unordered_map<std::string, std::string> fieldlist = deffields_, int offset = 0, int delay = 1, std::unordered_map<UpdateType, int, uthash> count = {{UpdateType::NORM, 0}, {UpdateType::INCR, 0}, {UpdateType::DECR, 0}, {UpdateType::RESET, 0}}, Status status = Status::OK, int statinfo = 0, bool fromdb = false);
	static Card &create(Deck &deck);
	static std::list<Card> &cards() { return cards_; }
	static void del(Card &card, bool refresh = true, bool explic = false);
private:
	int id_;
	Deck *deck_;
	int step_;
	int delay_;
	std::unordered_map<UpdateType, int, uthash> count_;
	Status status_;
	std::unordered_map<std::string, std::string> fields_;
	Card(int id, Deck *deck, std::unordered_map<std::string, std::string> fieldlist, int step, int delay, std::unordered_map<UpdateType, int, uthash> count, Status status, int statinfo) : id_{id}, deck_{deck}, step_{step}, delay_{delay}, count_{count}, status_{status}, fields_{fieldlist} { }
public:
	Card() = delete;
	Card(const Card &orig) = delete;
	Card(const Card&& orig) : id_{orig.id_}, deck_{orig.deck_}, step_{orig.step_}, delay_{orig.delay_}, count_{std::move(orig.count_)}, status_{orig.status_}, fields_{std::move(orig.fields_)} { }
	Card operator =(const Card& orig) = delete;
	virtual ~Card() { }
	
	std::string field(std::string name) const { return fields_.at(name); }
	int id() const { return id_; }
	int delay() const { return delay_; }
	Deck *deck() const { return deck_; }
	std::vector<std::string> vectorize(const std::vector<coldesc> &colspec) const;
	std::string display(std::vector<Field> fields) const;
	bool hasfield(std::string name) const { return fields_.count(name); }
	int offset() const;
	std::unordered_map<UpdateType, int, uthash> count() const { return count_; }
	int step() const { return step_; }
	Status status() const { return status_; }
	bool match(std::string query) const;
	bool avail() const;
	bool due(int diff = 0) const;
	
	void shift(int diff);
	void edit(Deck &deck, int offset, int delay, Status status);
	void field(std::string name, std::string value) { fields_.at(name) = value; backend::card_edit(*this, name); }
	void update(UpdateType type);
	friend bool operator ==(const Card &a, const Card &b) { return a.id_ == b.id_; }
};

#endif	/* CARD_H */

