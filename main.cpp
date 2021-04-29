
#include "gitsystem.hpp"


using namespace std;


int main(int argc, char* argv[])
{
	//sys::GIT_Start(argv[0]);
	//sys::GIT_Init();
	//sys::GIT_Status();
	//sys::GIT_Commit();

	//sys::GIT_Start(argv[0]);

	
	string txt_content; Strings txt_lines;
	sys::funcs::readMappFile("1.txt", txt_content);
	sys::funcs::splitString(txt_content, txt_lines);

	string txt_content1; Strings txt_lines1;
	sys::funcs::readMappFile("2.txt", txt_content1);
	sys::funcs::splitString(txt_content1, txt_lines1);

	//cout << txt_content << endl << "---------------------------" << txt_content1 << endl << endl;

	auto diffs = myers::get_diff(txt_lines, txt_lines1);
	

	sys::funcs::writeDiffsTo("1_2.diffs", diffs);
	diffs.diffs.clear();
	sys::funcs::readDiffsForm("1_2.diffs", diffs);
	myers::merage_diff(txt_lines, diffs);
	for (auto& x : txt_lines)
	{
		cout << x << endl;
	}


	


	return 0;
}