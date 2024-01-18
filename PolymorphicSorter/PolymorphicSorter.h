#ifndef POLYMORPHIC_SORTER_
#define POLYMORPHIC_SORTER_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <memory>
#include <fstream>
#include <unordered_map>
#include <map>
#include <set>
#include <functional>
#include <tuple>


class Writer
{
public:
	enum class OutputType
	{
		Stdout,
		File
	};

	Writer(OutputType outputType, const std::string& fileName = "");

	void writeLine(const std::string& line) const
	{
		(*outputStream_) << line << std::endl;
	}

	void write(const std::string& text) const
	{
		(*outputStream_) << text;
	}

private:
	OutputType outputType_;
	std::unique_ptr<std::ostream> outputStream_;
};

class Reader
{
public:
	enum class InputType
	{
		Stdin,
		File
	};

	Reader(InputType inputType_, const std::string& fileName = "");

	std::string readLine() const
	{
		std::string line;
		std::getline(*inputStream_, line);
		return line;
	}

	bool canRead() const
	{
		return !inputStream_->eof();
	}

private:
	InputType inputType_;
	std::unique_ptr<std::istream> inputStream_;
};

class AbstractValue
{
public:
	virtual void print(const Writer& writer) const = 0;
	virtual std::unique_ptr<AbstractValue> clone() = 0;
	virtual int compare(const AbstractValue& other) const = 0;
private:

};

enum ValueType
{
	Int = 1,
	String = 2,
	UnlimitedFloat = 3,
	FreqString = 4
};

struct MaxCmp
{
	bool operator()(std::pair<char, int> lhs, std::pair<char, int> rhs) const
	{
		if (lhs.second > rhs.second) return true;
		if (lhs.second < rhs.second) return false;
		return lhs.first < rhs.first;
	}
};

using valuePtr = std::unique_ptr<AbstractValue>;
using numberCList = const std::vector<int>&;
using valuePtrCList = std::vector<valuePtr>;
using arguments = std::vector < std::string>;
using TypeMap = std::unordered_map<ValueType, std::function<valuePtr(const std::string&)>>;
using columnsTypes = std::unordered_map<int, ValueType>;
using Table = std::vector<std::vector<valuePtr>>;
using freq = std::set<std::pair<char, int>, MaxCmp>;

class IntValue : public AbstractValue
{
public:
	IntValue(int x) { value_ = x; }
	virtual void print(const Writer& writer) const
	{
		writer.write(std::to_string(value_));
	}
	virtual valuePtr clone() override
	{
		return std::make_unique<IntValue>(*this);
	}
	virtual int compare(const AbstractValue& other) const override
	{
		auto intValue = dynamic_cast<const IntValue*>(&other);
		if (intValue)
		{
			if (value_ < intValue->value_)
				return -1;
			if (value_ == intValue->value_)
				return 0;
			else
				return 1;
		}
		return -2;
	}
private:
	int value_;
};

class StringValue : public AbstractValue
{
public:
	StringValue(std::string x) { value_ = x; }
	virtual void print(const Writer& writer) const
	{
		writer.write(value_);
	}
	valuePtr clone() override
	{
		return std::make_unique<StringValue>(*this);
	}
	virtual int compare(const AbstractValue& other) const override
	{
		auto stringValue = dynamic_cast<const StringValue*>(&other);
		if (stringValue)
		{
			if (value_ < stringValue->value_)
				return -1;
			if (value_ == stringValue->value_)
				return 0;
			else
				return 1;
		}
		return -2;
	}
private:
	std::string value_;
};
	
class UnlimitedFloatValue : public AbstractValue
{
public:
	UnlimitedFloatValue(std::string x);
	virtual void print(const Writer& writer) const
	{
		std::string output = (signed_) ? "-" : "";
		output += intPart_;
		output += (fractionPart_.empty()) ? "" : "." + fractionPart_;
		writer.write(output);
	}
	virtual valuePtr clone() override
	{
		return std::make_unique<UnlimitedFloatValue>(*this);
	}
	virtual int compare(const AbstractValue& other) const override;
private:
	int comapareIntPart_(const UnlimitedFloatValue* other) const;
	int comapareFractionPart_(const UnlimitedFloatValue* other) const;
	bool signed_; // true if < 0
	std::string intPart_;
	std::string fractionPart_;
};

class FreqStringValue : public AbstractValue
{
public:
	FreqStringValue(std::string x);
	virtual void print(const Writer& writer) const
	{
		writer.write(value_);
	}
	valuePtr clone() override
	{
		return std::make_unique<FreqStringValue>(*this);
	}
	virtual int compare(const AbstractValue& other) const override;
private:
	std::string value_;
	freq letterFrequency_;
};

static TypeMap typeMap = {
  { ValueType::Int, [](const std::string& value) { return std::make_unique<IntValue>(std::stoi(value)); } },
  { ValueType::String, [](const std::string& value) { return std::make_unique<StringValue>(value); } },
  { ValueType::UnlimitedFloat, [](const std::string& value) { return std::make_unique<UnlimitedFloatValue>(value); } },
  { ValueType::FreqString, [](const std::string& value) { return std::make_unique<FreqStringValue>(value); } }
};


class vectorCompare
{
public:
	vectorCompare(numberCList sortPriority) : sortPriority_(sortPriority) {}
	bool operator() (const valuePtrCList& v1, const valuePtrCList& v2) const;
private:
	numberCList sortPriority_;
};

class ArgumentChecker
{
public:
	ArgumentChecker(const arguments& args) : args_(args) {}
	int checkArguments();
	Writer createWriter();
	Reader createReader();
	std::string getSeparator();
	columnsTypes getColumnsTypes();
	std::vector<int> getSortingOrder();

private:
	std::tuple<int, int> checkIOArgument_(size_t index, bool nextItemIndicator, bool inputOutputIndicator);
	const arguments& args_;
	Writer::OutputType oType_ = Writer::OutputType::Stdout;
	Reader::InputType iType_ = Reader::InputType::Stdin;
	std::string iFileName_ = "";
	std::string oFileName_ = "";
	columnsTypes types_;
	std::string separator_ = " ";
	std::vector<bool> parametersIndicator_ = { false, false, false };
	std::unordered_map<char, ValueType> typeNames_ = { { 'N', Int }, { 'S', String}, {'U', UnlimitedFloat}, {'F', FreqString} };
	std::vector<int> sortingOrder_;

};

class Controller
{
public:
	Controller(const Writer& writer, const Reader& reader, columnsTypes& columnsTypes, const std::string& separator, const std::vector<int>& sortingOrder);
	int process();
private:
	const Writer& writer_;
	const Reader& reader_;
	columnsTypes& columnsTypes_;
	const std::string& separator_;
	int read_();
	int write_();
	int tablePushBack_(const arguments& tokens);
	size_t columnCount_ = 0;
	Table table_;
	valuePtrCList currentLine_;
	arguments splitString(const std::string& line);
	const std::vector<int>& sortingOrder_;
};


#endif // !POLYMORPHIC_SORTER_