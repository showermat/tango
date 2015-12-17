/* 
 * File:   Set.h
 * Author: matt
 *
 * Created on December 9, 2013, 3:04 PM
 */

#ifndef SET_H
#define	SET_H

#include <deque>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include "Card.h"

class Set
{
public:
	enum class SetType { NORMAL = 0, ALL, KANJI, KANA }; // TODO Most recently missed, missed in the last n days, etc. // Ensure NORMAL is always first!
	enum class DispType { FRONT = 0, BACK, HINT };
	struct sthash { size_t operator ()(const SetType &x) const { return static_cast<size_t>(x); } };
	struct dthash { size_t operator ()(const DispType &x) const { return static_cast<size_t>(x); } };
	struct vfhash { size_t operator ()(const std::vector<Card::Field> &x) const { size_t ret = 0; for (const Card::Field f : x) ret += static_cast<size_t>(f); return ret; } };
	static const std::vector<SetType> settypes();
	static std::string st2str(SetType type)
	{
		if (type == SetType::NORMAL) return "Normal";
		if (type == SetType::ALL) return "All";
		if (type == SetType::KANJI) return "Kanji";
		if (type == SetType::KANA) return "Kana";
		throw std::runtime_error{"Impossible error in st2str"};
	}
	static SetType str2st(std::string str)
	{
		if (str == "Normal") return SetType::NORMAL;
		if (str == "All") return SetType::ALL;
		if (str == "Kanji") return SetType::KANJI;
		if (str == "Kana") return SetType::KANA;
		throw std::runtime_error{"Invalid string " + str + " passed to str2st"};
	}
	static std::unordered_map<SetType, std::unordered_map<DispType, std::unordered_set<std::vector<Card::Field>, vfhash>, dthash>, sthash> defdisp();
private:
	std::deque<Card *> items_;
	std::deque<Card *> repeats_;
	SetType type_;
	Card *top_;
	Deck *deck_;
	std::unordered_map<DispType, std::unordered_set<std::vector<Card::Field>, vfhash>, dthash> displays_;
	std::unordered_map<DispType, std::vector<Card::Field>, dthash> curdisp_;
	void remove(Card *card);
public:
	Set() = delete;
	Set(Deck *deck, SetType type);
	Set (const Set &orig) = delete;
	Set(Set &&orig) : items_{std::move(orig.items_)}, repeats_{orig.repeats_}, type_{orig.type_}, top_{orig.top_}, deck_{orig.deck_}, displays_{orig.displays_}, curdisp_{} { }
	Set operator =(const Set& orig) = delete;
	virtual ~Set() { }
	
	std::string canonical() const;
	Deck &deck() const { return *deck_; }
	int size(bool repeats = true) const;
	Card &top();
	std::string disptop(DispType type);
	
	void deck(Deck *d) { deck_ = d; }
	void refresh();
	void add(Card *card) { items_.push_back(card); }
	void empty() { clear(); items_.clear(); }
	void shuffle();
	void clear();
	void update(Card::UpdateType ut);
};

namespace std { template <> struct hash<Set::SetType> { size_t operator ()(const Set::SetType &x) const { return static_cast<size_t>(x); } }; }

#endif	/* SET_H */

