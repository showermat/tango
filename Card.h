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
	static std::unordered_map<std::string, std::string> deffields;
	static std::unordered_map<std::string, std::string> fieldpref, fieldsuff;	
	static std::list<Card> cards_;
	static const double ratio;
	static const int maxdelay_;
	static int cardnum;
public:
	enum class Status { OK = 1, SUSP = 2, DONE = 3, LEECH = 4 };
	enum class UpdateType { NONE, INCR, DECR, NORM, RESET, SUSP, LEECH, BURY, RESUME, DONE };
	enum class Field { NONE, KANJI, HIRAGANA, FURIGANA, MEANING };
	static std::vector<std::string> fieldnames() { return fieldnames_; }
	static int maxdelay() { return maxdelay_; }
	static std::string stat2str(Status status);
	static Status str2stat(std::string status);
	static std::string sit2str(Field sit);
	static std::string sits2str(std::vector<Field> sits);
	static Field str2sit(std::string str);
	static std::string html_furigana(std::string kanji, std::string furigana);
	static std::string hiragana(std::string kanji, std::string furigana);
	static Card &add(Deck &deck, int id = ++cardnum, std::unordered_map<std::string, std::string> fieldlist = deffields, int offset = 0, int delay = 1, Status status = Status::OK, int statinfo = 0, bool fromdb = false);
	static Card &create(Deck &deck);
	static std::list<Card> &cards() { return cards_; }
	static void del(Card &card, bool refresh = true, bool explic = false);
private:
	int id_;
	Deck *deck_;
	int offset_;
	int delay_;
	Status status_;
	std::unordered_map<std::string, std::string> fields_;
	Card(int id, Deck *deck, std::unordered_map<std::string, std::string> fieldlist, int offset, int delay, Status status, int statinfo);
public:
	Card() = delete;
	Card(const Card &orig) = delete;
	Card(const Card&& orig) : id_{orig.id_}, deck_{orig.deck_}, offset_{orig.offset_}, delay_{orig.delay_}, status_{orig.status_}, fields_{std::move(orig.fields_)} { }
	Card operator =(const Card& orig) = delete;
	virtual ~Card() { }
	
	std::string field(std::string name) const { return fields_.at(name); }
	int id() const { return id_; }
	int delay() const { return delay_; }
	Deck *deck() const { return deck_; }
	std::vector<std::string> vectorize(const std::vector<coldesc> &colspec) const;
	std::string display(std::vector<Field> fields) const;
	bool hasfield(std::string name) const { return fields_.count(name); }
	int offset() const { return offset_; }
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

//namespace std { template <> struct hash<Card> { size_t operator ()(const Card &x) const { return static_cast<size_t>(x.id()); } }; }

#endif	/* CARD_H */

