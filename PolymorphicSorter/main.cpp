
#include "PolymorphicSorter.h"

using namespace std;

int main(int argc, char** argv)
{
	arguments args(argv + 1, argv + argc);
	ArgumentChecker checker(args);
	int success = checker.checkArguments();
	if (success != 0)
	{
		cerr << "Wrong arguments" << endl;
		return 0;
	}
		
	auto writer = checker.createWriter();
	auto reader = checker.createReader();
	auto separator = checker.getSeparator();
	auto columnsTypes = checker.getColumnsTypes();
	auto sortingOrder = checker.getSortingOrder();

	Controller controller(writer, reader, columnsTypes, separator, sortingOrder);
	success = controller.process();
	if (success != 0)
		cerr << "Exit code " << success << endl;;

	return 0;
}