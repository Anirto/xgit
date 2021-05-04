
#include "gitsystem.hpp"


using namespace std;

// ���Ժ���
static void test()
{
	// �����ı�
	ofstream out1("1.txt");
	out1 << "1111\n\n4444\n5555" << endl;

	ofstream out2("2.txt");
	out2 << "1111\n\n\n5555\n6666\n9999" << endl;

	out1.close();
	out2.close();

	// ��ȡ�ı�
	string txt_content; Strings txt_lines;
	sys::funcs::readMappFile("1.txt", txt_content);
	sys::funcs::splitString(txt_content, txt_lines);

	string txt_content1; Strings txt_lines1;
	sys::funcs::readMappFile("2.txt", txt_content1);
	sys::funcs::splitString(txt_content1, txt_lines1);

	// ��������
	auto diffs = myers::get_diff(txt_lines, txt_lines1);

	// ���л���д�������ļ���
	sys::funcs::writeDiffsTo("1_2", diffs);
	diffs.diffs.clear();

	// �����л���������
	sys::funcs::readDiffsForm("1_2", diffs);

	// �ϲ�������
	myers::merage_diff(txt_lines, diffs);

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
	sys::GIT_Start("D:\\Repo\\xgit\\test\\");
	string cmd;

	do
	{
		cout << "D:\\Repo\\xgit\\test" << endl << ">>";
		cin >> cmd;
		if (cmd == "init")
			sys::GIT_Init();
		else if (cmd == "status")
			sys::GIT_Status();
		else if (cmd == "commit")
			sys::GIT_Commit();
		else if (cmd == "reset")
			sys::GIT_Reset();
		else if (cmd == "exit")
			break;
		else if (cmd == "help")
			help();
		else
			cerr << "unknown command" << endl;
		cout << endl;
	} while (1);

	//test();
	

	return 0;
}