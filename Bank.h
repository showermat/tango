/* 
 * File:   Bank.h
 * Author: matt
 *
 * Created on December 2, 2013, 5:02 PM
 */

#ifndef BANK_H
#define	BANK_H

#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include "Card.h"
#include "backend.h"

class Deck;

class Bank
{
private:
	struct BankItem
	{
		int offset;
		unsigned int practices;
		BankItem(int off, unsigned int p) : offset{off}, practices{p} { }
	};
	struct Section
	{
		std::string name;
		std::unordered_set<std::string> words;
		Section(std::string n) : name{n}, words{} { }
		void add(std::string word) { words.insert(word); }
		void del(std::string word) { if (words.count(word)) words.erase(words.find(word)); }
	};
	static std::vector<std::string> tokenize(std::string str);
	
	std::unordered_map<std::string, BankItem> words_;
	std::unordered_set<std::string> inset_;
	std::string field_;
	Deck *deck_;
	std::list<Section> basis_;
	Section &sect(std::string name);
public:
	void addsect(std::string name) { sect(name); }
	void add(std::string section, std::string word) { sect(section).add(word); }
	void del(std::string word) { for (Section &section : basis_) section.del(word); }
	Bank() : Bank{nullptr, ""} { }
	Bank(Deck *deck, std::string field) : words_{}, inset_{}, field_{field}, deck_{deck}, basis_{} { }
	Bank(const Bank& orig) = default;
	Bank &operator =(const Bank& orig) = default;
	virtual ~Bank() { }
	
	int size() const { return words_.size(); }
	Deck *deck() const { return deck_; }
	bool enabled(std::string word) const;
	std::vector<std::string> vectorize(std::string word, const std::vector<coldesc> &colspec) const;
	std::vector<std::string> words() const;
	std::string htmlview() const;
	void commit(std::string olddeck = "") const;
	bool check(Card *card);
	
	void deck(Deck *d) { deck_ = d; }
	void field(std::string f) { field_ = f; }
	bool enable(std::string word, int offset = 0, unsigned int n = 0, bool fromdb = false);
	bool disable(std::string word);
	void shift(int diff);
	void clear() { inset_.clear(); }
	void update(Card *card, Card::UpdateType type);
};

#endif	/* BANK_H */

