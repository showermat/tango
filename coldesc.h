/* 
 * File:   ColDesc.h
 * Author: matt
 *
 * Created on September 5, 2013, 11:03 AM
 */

#ifndef COLDESC_H
#define	COLDESC_H

#include <string>
#include <vector>
#include "util.h"

struct coldesc
{
	enum class Type { INT, BOOL, STRING, FIELD, CHOICE, DATE, STATIC_INT };
	Type type;
	std::string title;
	std::vector<std::string> choices;
	int min, max;
	int width;
	coldesc(Type type_, std::string desc, int width_ = -1)
	{
		type = type_;
		width = width_;
		title = util::strto(desc, ":");
		if (type == Type::CHOICE) while (desc != "") choices.push_back(util::strto(desc, ","));
		else if (type == Type::INT)
		{
			std::string min_ = util::strto(desc, ",");
			std::string max_ = util::strto(desc, ",");
			if (min_ == "") min = 0;
			else min = util::s2t<int>(min_);
			if (max_ == "") max = 1000000000;
			else max = util::s2t<int>(max_);
		}
	}
};

#endif	/* COLDESC_H */

