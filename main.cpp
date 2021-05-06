
#include "gitsystem.hpp"



using namespace std;

// 测试函数
static void test()
{
	// 生成文本
	ofstream out1("1.txt");
	out1 << "1111\n\n4444\n5555" << endl;

	ofstream out2("2.txt");
	out2 << "1111\n\n\n5555\n6666\n9999" << endl;

	out1.close();
	out2.close();

	// 读取文本
	string txt_content; Strings txt_lines;
	sys::funcs::readMappFile("1.txt", txt_content);
	sys::funcs::splitString(txt_content, txt_lines);

	string txt_content1; Strings txt_lines1;
	sys::funcs::readMappFile("2.txt", txt_content1);
	sys::funcs::splitString(txt_content1, txt_lines1);

	// 计算增量
	auto diffs = myers::getDiffs(txt_lines, txt_lines1);

	// 序列化，写增量到文件中
	sys::funcs::writeDiffsTo("1_2", diffs);
	diffs.diffs.clear();

	// 反序列化，读增量
	sys::funcs::readDiffsForm("1_2", diffs);

	// 合并增量，
	myers::merageDiff(txt_lines, diffs);

	for (auto& x : txt_lines)
	{
		cout << x << endl;
	}

}

static void help()
{
	cout << "using [command] :" << endl
		<< "help    if you dont konw how to use this software, just input \'help\'\n" << endl
		<< "init    this command will create repository in current directory\n" << endl
		<< "status  using \'status\' command to show which files have changed\n" << endl
		<< "commit  after you input \'commit\', all changed files " << endl
		<< "        including deleted and added files, will be recorded\n" << endl
		<< "reset   if you accidentally change or delete some files," << endl
		<< "        you can use this command if you want to undo the change\n" << endl
		<< "exit    exit this softeare\n" << endl;
}



int main(int argc, char* argv[])
{
	sys::GIT_Start(TESTDIR);
	string cmd;

	do
	{
		cout << TESTDIR << endl << ">>";
		cin >> cmd;
		if (cmd == "init")
			sys::GIT_Init();
		else if (cmd == "status")
			sys::GIT_Status();
		else if (cmd == "commit")
			sys::GIT_Commit();
		else if (cmd == "reset")
			sys::GIT_Reset();
		else if (cmd == "help")
			help();
		else if (cmd == "exit")
			break;
		else
			cerr << "unknown command" << endl;
		cout << endl;
	} while (1);

	//test();
	

	return 0;
}