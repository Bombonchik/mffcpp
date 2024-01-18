#include "PolymorphicSorter.h"

using namespace std;

bool is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

bool vectorCompare::operator() (const valuePtrCList& v1, const valuePtrCList& v2) const
{
	for (auto&& index : sortPriority_)
	{
		auto result = v1[index]->compare(*v2[index]);
		if (result == -2)
			return false;
		if (result == -1)
			return true;
		if (result == 1)
			return false;
	}
	return false;
}

Writer::Writer(OutputType outputType, const std::string& fileName) : outputType_(outputType)
{
	if (outputType_ == OutputType::File)
		outputStream_ = std::make_unique<std::ofstream>(fileName);
	else
		outputStream_ = std::make_unique<std::ostream>(std::cout.rdbuf());
}

Reader::Reader(InputType inputType_, const std::string& fileName) : inputType_(inputType_)
{
	if (inputType_ == InputType::File)
		inputStream_ = std::make_unique<std::ifstream>(fileName);
	else
		inputStream_ = std::make_unique<std::istream>(std::cin.rdbuf());
}

//inputOutputIndicator - false(input), true(output)
//tuple<int,int> first is exit code, second index
std::tuple<int, int> ArgumentChecker::checkIOArgument_(size_t index, bool nextItemIndicator, bool inputOutputIndicator)
{
	std::fstream file;
	string fileName;
	int currentArgumentCount;
	const int argsNameLength = 2;
	int parameterIndicatorIndex = (inputOutputIndicator) ? 1 : 0;

	if (parametersIndicator_[parameterIndicatorIndex]) // check if it already was
		return std::make_tuple(10,0);
	if (args_[index].length() > argsNameLength)
	{
		fileName = args_[index].substr(argsNameLength);
		currentArgumentCount = 1;
	}
	else
	{
		if (!nextItemIndicator)
			return std::make_tuple(12, 0);
		currentArgumentCount = 2;
		fileName = args_[index + 1];
	}
	file.open(fileName, std::fstream::in | std::fstream::app);
	if (file)
	{
		parametersIndicator_[parameterIndicatorIndex] = true;
		if (!inputOutputIndicator)
		{
			iFileName_ = fileName;
			iType_ = Reader::InputType::File;
		}
		else
		{
			oFileName_ = fileName;
			oType_ = Writer::OutputType::File;
		}
		file.close();
	}
	else
		return std::make_tuple(14, 0);
	return std::make_tuple(0, currentArgumentCount - 1);
}

int ArgumentChecker::checkArguments()
{
	bool checkingOptionalParametersIndicator = true;
	for (size_t i = 0; i < args_.size(); i++)
	{
		if (checkingOptionalParametersIndicator)
		{
			bool nextItemIndicator = (i + 1 < args_.size());
			auto argumentName = args_[i].substr(0, 2);
			if (argumentName == "-i" || argumentName == "-o")
			{
				bool inputOutputIndicator = (argumentName == "-o");
				tuple<int, int> result = checkIOArgument_(i, nextItemIndicator, inputOutputIndicator);
				int exitCode = std::get<0>(result);
				if (exitCode != 0)
					return exitCode + inputOutputIndicator;
				i += std::get<1>(result);
				continue;
			}
			if (argumentName == "-s")
			{
				if (parametersIndicator_[2])
					return 16;
				if (args_[i].length() > 2)
					separator_ = args_[i].substr(2);
				else
				{
					if (nextItemIndicator && args_[i + 1].length() != 1)
						return 17;
					separator_ = args_[i + 1];
					i++;
				}
				parametersIndicator_[2] = true;
				continue;
			}
			checkingOptionalParametersIndicator = false;
		}
		if (!checkingOptionalParametersIndicator)
		{
			auto currentValueType = typeNames_.find(args_[i][0]);
			if (currentValueType == typeNames_.end())
				return 18;
			string positionS = args_[i].substr(1);
			if (!is_number(positionS))
				return 19;
			int position = stoi(positionS) - 1;
			types_[position] = currentValueType->second;
			sortingOrder_.push_back(position);
		}
	}
	return 0;
}

Writer ArgumentChecker::createWriter()
{
	return Writer(oType_, oFileName_);
}
Reader ArgumentChecker::createReader()
{
	return Reader(iType_, iFileName_);
}
std::string ArgumentChecker::getSeparator()
{
	return separator_;
}
columnsTypes ArgumentChecker::getColumnsTypes()
{
	return types_;
}

std::vector<int> ArgumentChecker::getSortingOrder()
{
	return sortingOrder_;
}

Controller::Controller(const Writer& writer, const Reader& reader, columnsTypes& columnsTypes, const std::string& separator, const std::vector<int>& sortingOrder) :
	writer_(writer), reader_(reader), columnsTypes_(columnsTypes), separator_(separator), sortingOrder_(sortingOrder)
{

}

int Controller::process()
{
	int success = read_();
	if (success != 0)
		return success;
	if (!sortingOrder_.empty())
		sort(table_.begin(), table_.end(), vectorCompare{ sortingOrder_ });
	success = write_();
	if (success != 0)
	{
		cerr << "It's impossible to write to file" << endl;
		return success;
	}
	return 0;
}

int Controller::read_()
{
	if (reader_.canRead())
	{
		std::string line = reader_.readLine();
		arguments tokens = splitString(line);
		columnCount_ = tokens.size();
		for (auto x : sortingOrder_)
		{
			if (columnCount_ - 1 < x)
			{
				std::cerr << "Column out of range: " << x << std::endl;
				return -1;
			}
		}

		for (size_t i = 0; i < columnCount_; i++)
		{
			auto it = columnsTypes_.find(i);
			if (it == columnsTypes_.end())
				columnsTypes_[i] = String;
		}
		int success = tablePushBack_(tokens);
		if (success != 0)
			return success;

	}
	while (reader_.canRead())
	{
		std::string line = reader_.readLine();
		arguments tokens = splitString(line);
		int success = tablePushBack_(tokens);
		if (success != 0)
			return success;
	}
	return 0;
}

arguments Controller::splitString(const string& line)
{
	arguments tokens;
	int start = 0;
	int end = line.find(separator_);
	while (end != -1) {
		auto word = line.substr(start, end - start);
		tokens.push_back(word);
		start = end + separator_.size();
		end = line.find(separator_, start);
	}
	tokens.push_back(line.substr(start, end - start));
	return tokens;
}

int Controller::tablePushBack_(const arguments& tokens)
{
	size_t rowCount = table_.size();
	if (tokens.size() != columnCount_)
		return tokens[0].size();
	for (size_t i = 0; i < tokens.size(); i++)
	{
		try
		{
			auto it = typeMap.find(columnsTypes_[i]);
			currentLine_.push_back(it->second(tokens[i]));
		}
		catch (const std::invalid_argument& e)
		{
			std::cerr << "error: line " << rowCount + 1 << ", column " << i + 1 << " - invalid format " << std::endl;
			return 22;
		}
		catch (const std::out_of_range& e)
		{
			std::cerr << "Out of range: " << e.what() << std::endl;
			return 23;
		}
	}
	table_.push_back(move(currentLine_));
	return 0;
}

int Controller::write_()
{
	for (size_t i = 0; i < table_.size(); i++)
	{
		for (size_t j = 0; j < columnCount_ - 1; j++)
		{
			table_[i][j]->print(writer_);
			writer_.write(separator_);
		}
		table_[i][columnCount_ - 1]->print(writer_);
		writer_.write("\n");
	}
	return 0;
}

UnlimitedFloatValue::UnlimitedFloatValue(std::string x) : signed_(false) {
	size_t firstDigitIndex = 0;
	if (x[0] == '-' && x.size() > 1)
	{
		firstDigitIndex = 1;
		signed_ = true;
	}
	auto dot = x.find('.');
	if (dot != std::string::npos) //this is float
	{
		size_t lastIndex = x.size() - 1;
		if (dot != lastIndex)
			fractionPart_ = x.substr(dot + 1, lastIndex - dot);
		intPart_ = x.substr(firstDigitIndex, dot - firstDigitIndex);
	}
	else
	{
		intPart_ = x.substr(firstDigitIndex);
	}
}

int UnlimitedFloatValue::compare(const AbstractValue& other) const
{
	auto unlimitedFloatValue = dynamic_cast<const UnlimitedFloatValue*>(&other);
	if (unlimitedFloatValue)
	{
		if (signed_ && !unlimitedFloatValue->signed_)
			return -1;
		if (!signed_ && unlimitedFloatValue->signed_)
			return 1;
		int bothSigned = (signed_ && unlimitedFloatValue->signed_) ? -1 : 1;
		int result = comapareIntPart_(unlimitedFloatValue);
		if (result != 0) return bothSigned * result;
		result = comapareFractionPart_(unlimitedFloatValue);
		return bothSigned * result;
	}
	return -2;
}

int UnlimitedFloatValue::comapareIntPart_(const UnlimitedFloatValue* other) const
{
	if (intPart_.size() < other->intPart_.size())
		return -1;
	if (intPart_.size() > other->intPart_.size())
		return 1;
	for (size_t i = 0; i < intPart_.size(); i++)
	{
		if (intPart_[i] < other->intPart_[i])
			return -1;
		if (intPart_[i] > other->intPart_[i])
			return 1;
	}
	return 0;
}

int UnlimitedFloatValue::comapareFractionPart_(const UnlimitedFloatValue* other) const
{
	auto minSize = (fractionPart_.size() < other->fractionPart_.size()) ? fractionPart_.size() : other->fractionPart_.size();
	for (size_t i = 0; i < minSize; i++)
	{
		if (fractionPart_[i] < other->fractionPart_[i])
			return -1;
		if (fractionPart_[i] > other->fractionPart_[i])
			return 1;
	}
	if (fractionPart_.size() < other->fractionPart_.size()) return -1;
	if (fractionPart_.size() > other->fractionPart_.size()) return 1;
	return 0;
}


FreqStringValue::FreqStringValue(std::string x) : value_(x) {
	std::unordered_map<char, int> helpMap;

	for (auto letter : value_)
	{
		auto fr = helpMap.find(letter);
		if (fr != helpMap.end())  //found
			fr->second++;
		else
		{
			helpMap[letter] = 1;
		}
	}
	for (auto it : helpMap)
	{
		letterFrequency_.insert(std::make_pair(it.first, it.second));
	}
}

int FreqStringValue::compare(const AbstractValue& other) const
{
	auto freqStringValue = dynamic_cast<const FreqStringValue*>(&other);
	if (freqStringValue)
	{
		auto it = letterFrequency_.begin();
		auto otherIt = freqStringValue->letterFrequency_.begin();
		while (it != letterFrequency_.end() && otherIt != freqStringValue->letterFrequency_.end())
		{
			if (it->second > otherIt->second) return -1; //Pokud max_f(s1) > max_f(s2), potom s1 < s2
			if (it->second < otherIt->second) return 1; //Pokud max_f(s1) < max_f(s2), potom s1 > s2
			if (it->first < otherIt->first) return -1;
			if (it->first > otherIt->first) return 1;
			it++;
			otherIt++;
		}
	}
	return -2;
}
