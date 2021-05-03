#pragma once
#include "xgitconfig.h"
#include "algorithm.hpp"
#include "serialize.hpp"


using std::string;

//******************************* ���建��ṹ ********************************//
struct FileStatu
{
	long long mtime;
	unsigned short mode;
	SHINE_SERIAL(FileStatu, mtime, mode);
};

class File
{
public:
	bool operator < (const File& other) const
	{
		return this->nameHash < other.nameHash;
	}

public:
	HashValue nameHash;
	HashValue fileHash;
	FileStatu fileStat;
	string    filename;
	uint32_t  version;

	SHINE_SERIAL(File, nameHash, fileHash, filename, fileStat, version);
};

class Caches
{
public:
	// waring : ɾ��Ԫ��ʱ��������ʧЧ
	std::set<File>	files;		// �ļ�״̬�б�
	uint32_t		git_version;// �汾��
	
	SHINE_SERIAL(Caches, files, git_version);
};

class Version
{
public:
	Strings cgd_files;
	SHINE_SERIAL(Version, cgd_files);
};

//********************************* end ***********************************//

namespace sys {
	
	//           .\.git\cache  .\.git\objs  .\ 
	static string cache_path, cache_dir, curr_dir;
	/*
		---cache
		---objs
		   |--- 0x123456789
				|----------0x123456789
				|----------0
				|----------1
				|----------2
				...
			...
	*/


	namespace
	{
		enum status { ADD, DEL, MODIFY };
		struct Change
		{
			status mode;
			Strings::const_iterator iter;
		};

	}

	using Changes = std::vector<Change>;


	namespace funcs
	{
		using std::string;

		/**
		* ʹ���ڴ�ӳ���ȡ�ļ���
		*/
		bool readMappFile(const string& name, string& out)
		{
			// �ڴ�ӳ��
			HANDLE hfile = CreateFile(name.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			auto ttt = GetLastError();
			assert(hfile);
			assert(!(hfile == INVALID_HANDLE_VALUE));

			if (hfile == NULL || hfile == INVALID_HANDLE_VALUE)
			{
				std::cerr << "fatal: read file failed " << name << std::endl;
				return false;
			}
				

			HANDLE mapobj = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
			void* mapfile = MapViewOfFile(mapobj, FILE_MAP_READ, 0, 0, GetFileSize(hfile, NULL));

			out = (static_cast<char*>(mapfile));

			UnmapViewOfFile((LPVOID)mapfile);
			CloseHandle(mapobj);
			CloseHandle(hfile);

			return true;

		}
		
		/**
		* �Ի��з��ָ��ַ���
		*/
		bool splitString(const string& fileContext, Strings& out)
		{

			std::string::size_type lastpos = fileContext.find_first_not_of("\n", 0);
			std::string::size_type currpos = fileContext.find_first_of("\n", lastpos);

			while (currpos != string::npos || lastpos != string::npos)
			{
				out.emplace_back(fileContext.substr(lastpos, currpos - lastpos));
				lastpos = fileContext.find_first_not_of("\n", currpos);
				currpos = fileContext.find_first_of("\n", lastpos);
			}
			return true;
		}

		/**
		*  ��ȡ����
		*/
		bool readDiffsForm(const string& name, Diffs& out)
		{
			std::ifstream in(name, std::ios::binary);
			if (!in.is_open())
			{
				std::cerr << "read version files failed." << std::endl;
				return false;
			}
				
			std::ostringstream tmp;
			tmp << in.rdbuf();
			string buffer = tmp.str();
			in.close();
			out.shine_serial_decode(buffer);
			return true;
		}

		/**
		* д�������ļ���
		*/
		bool writeDiffsTo(const string& name, const Diffs& dif)
		{
			auto buffer = dif.shine_serial_encode();
			std::ofstream out(name, std::ios::binary);
			if (!out.is_open())
			{
				std::cerr << "write version files failed." << std::endl;
				return false;
			}
			out << buffer;
			out.close();
			return true;
		}

		/**
		 *  ����Ŀ¼
		 */
		bool makeDir(const string& path)
		{
			return false;
		}

		/**
		 * ��ȡ�����ļ�
		 */
		bool readCache(const string& path, string& buffer)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in.is_open())
			{
				//std::cerr << "fatal: read cache failed." << std::endl;
				in.close();
				return false;
			}
			else
			{
				std::ostringstream tmp;
				tmp << in.rdbuf();
				buffer = tmp.str();
				in.close();
			}
			return true;
		}

		/**
		 * д�����ļ�
		 */
		bool writeCache(const string& path, const string& buffer)
		{
			// write cache
			std::ofstream out(path, std::ios::trunc | std::ios::binary);
			if (!out.is_open())
			{
				std::cerr << "fatal: write cache failed." << std::endl;
				return false;
			}
			out << buffer;
			out.close();
			return true;
		}

		/**
		 * ��ȡ�ļ�״̬
		 */
		bool getFileStatu(const string& name, FileStatu& out)
		{
			int fd = _open(name.c_str(), O_RDONLY);
			if (fd < 0)
			{
				std::cerr << "fatal: get file statu failed : " << name << std::endl;
				_close(fd);
				return false;
			}

			struct  stat st;
			if (fstat(fd, &st) < 0)
				throw "Get file statu failed";

			out.mode = st.st_mode;
			out.mtime = st.st_mtime;
			return true;
		}
	};
};

// �ඨ���� // 
namespace sys 
{

	/**
	 * ����ģʽ
	 *		ά��������Ϣ
	 */
	class Git
	{
	public:
		static Git* getInstance()
		{
			static Git instance;
			return &instance;
		}

		/*
		* ��ʼ������ȡ����
		*	1. �����ж�ȡce
		*	2. �����������ļ��б� cach_files��������
		*	3. �����������ļ��� -> �ļ�ָ��ӳ��
		*/
		bool initial()
		{
			string buffer;
			if (funcs::readCache(cache_path, buffer) == false)
				return false;

			ce.shine_serial_decode(buffer);

			// ����ce���ļ��б� ���ļ�ָ��ӳ��
			for (auto it = ce.files.begin(); it != ce.files.end(); it++)
			{
				cach_files.emplace_back(it->filename);
				hash_file[it->filename] = it;
			}
			std::sort(cach_files.begin(), cach_files.end());

			return true;
		}

		bool finish()
		{
			string buffer = version.shine_serial_encode();
			funcs::writeCache(curr_dir + "\\.git\\" + std::to_string(ce.git_version++), buffer);
			version.cgd_files.clear();
			buffer = ce.shine_serial_encode();
			return funcs::writeCache(cache_path, buffer);
		}

		/**
		 * ���һ���ļ���������
		 */
		bool addFile(const string& name)
		{
		
			string file_text;
			if (funcs::readMappFile(name, file_text) == false)
				return false;

			File file;
			if (funcs::getFileStatu(name, file.fileStat) == false)
				return false;

			if (file.fileStat.mode & S_IFMT != 0x0020000)
				return false;

			file.version = 0;
			file.filename = name;
			file.fileHash = austin::MurmurHash3(file_text.c_str(), file_text.size());
			file.nameHash = austin::MurmurHash3(name.c_str(), name.size());
			funcs::getFileStatu(name, file.fileStat);

			cach_files.push_back(name);
			hash_file[name] = ce.files.insert(file).first;
			std::sort(cach_files.begin(), cach_files.end());

			version.cgd_files.emplace_back(name);

			string write_dir = cache_dir + "\\" + file.nameHash.toString();
			if (_mkdir(write_dir.c_str()) < 0)
				return false;

			return funcs::writeCache(write_dir + "\\" + file.nameHash.toString(), file_text);
		}

		/**
		 * �ӻ�����ɾ��һ���ļ�
		 */
		bool removeFile(const string& name)
		{
			auto const_p_file = hash_file[name];
			auto nam_hash = austin::MurmurHash3(name.c_str(), name.size());

			//д����
			string incer_path = cache_dir + "\\" + nam_hash.toString() + "\\" + std::to_string(const_p_file->version);
			string incer_text; funcs::readCache(incer_path, incer_text);
			incer_path = cache_dir + "\\" + nam_hash.toString() + "\\" + std::to_string(const_p_file->version+1);
			funcs::writeCache(incer_path, incer_text);

			/*
			auto it = lower_bound(cach_files.begin(), cach_files.end(), name);
			cach_files.erase(it);
			ce.files.erase(const_p_file);
			hash_file.erase(name);
			*/

			// ���ļ�״̬
			auto pfile = const_cast<File*>(&(*const_p_file));
			pfile->version++;

			version.cgd_files.emplace_back(name);

			return true;
		}

		/**
		 * �޸Ļ����е��ļ�
		 */
		bool modifyFile(const string& name)
		{
			auto const_pfile = hash_file[name];

			// ��ȡԭʼ�汾
			string  origin_path = cache_dir + "\\" + const_pfile->nameHash.toString() + "\\" + const_pfile->nameHash.toString();
			string  origin_text; funcs::readMappFile(origin_path, origin_text);
			Strings origin_lins; funcs::splitString(origin_text, origin_lins);

			// ��ȡ��ǰ�汾
			string	curent_text; funcs::readMappFile(name, curent_text);
			Strings curent_lins; funcs::splitString(curent_text, curent_lins);

			// д����
			string incer_path = cache_dir + "\\" + const_pfile->nameHash.toString() + "\\" + std::to_string(const_pfile->version);
			auto diffs = myers::get_diff(origin_lins, curent_lins);
			funcs::writeDiffsTo(incer_path, diffs);

			// ���ļ�״̬
			auto pfile = const_cast<File*>(&(*const_pfile));
			pfile->version++;
			funcs::getFileStatu(name, pfile->fileStat);
			funcs::readMappFile(name, curent_text);
			pfile->fileHash = austin::MurmurHash3(curent_text.c_str(), curent_text.size());

			version.cgd_files.emplace_back(name);

			return true;
		}

		/**
		 * �ָ��ļ�����һ�汾
		 */
		bool resetFile(const string& name)
		{
			auto p_file = hash_file[name];

			auto origi_path = cache_dir + "\\" + p_file->nameHash.toString() + "\\" + p_file->nameHash.toString();
			auto incer_path = cache_dir + "\\" + p_file->nameHash.toString() + "\\" + std::to_string(p_file->version-2);
			string origin_text, incer_buff;
			funcs::readMappFile(origi_path, origin_text);

			if (p_file->version == 1)
				return funcs::writeCache(name, origin_text);
			
			Strings origin_lines;
			funcs::splitString(origin_text, origin_lines);

			Diffs diffs;
			funcs::readDiffsForm(incer_path, diffs);

			myers::merage_diff(origin_lines, diffs);
			
			std::ofstream out(name, std::ios::trunc | std::ios::binary);
			for (auto& line : origin_lines)
				out << line  << std::endl;
			out.close();
			(const_cast<File*>(&(*p_file)))->version--;
			return true;
		}
		

	private:
		Git() {};
		Git(Git&) = delete;
		Git& operator=(const Git&) = delete;
		

	public:
		/* �ļ�״̬�б� */
		Caches ce;

		/* �����У� �ļ����б� */
		Strings cach_files;

		/* �����У� �ļ��� -> �ļ�״ָ̬�� */
		std::map<string, std::set<File>::iterator> hash_file;

		/* �˴θĶ����ļ��б� */
		Version version;
	};

	/**
	 * ����ģʽ
	 * ��ǰĿ¼������
	 */
	class Cur
	{
	public:
		static Cur* getInstance()
		{
			static Cur instance;
			return &instance;
		}

		const Strings& getCurrFiles()
		{
			curr_files.clear();
			getFilesAll(curr_dir, curr_files);
			return curr_files;
		}

	private:
		Cur() {};
		Cur(Cur&) = delete;
		Cur& operator=(const Cur&) = delete;

	private:
		/* Ŀ¼�£� �ļ����б� */
		Strings curr_files;

		/* ��ȡĿ¼�������ļ��б� */
		void getFilesAll(string path, Strings& files)
		{
			//�ļ����
			long  hFile = 0;
			//�ļ���Ϣ
			struct _finddata_t fileinfo;
			string p;
			if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
			{
				do
				{
					if ((fileinfo.attrib & _A_SUBDIR))
					{
						if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
						{
							//files.push_back(p.assign(path).append("\\").append(fileinfo.name) );
							getFilesAll(p.assign(path).append("\\").append(fileinfo.name), files);
						}
					}
					else
					{
						if (path.find(".git") == string::npos)
							files.push_back(p.assign(path).append("\\").append(fileinfo.name));
					}
						

				} while (_findnext(hFile, &fileinfo) == 0);
				_findclose(hFile);
			}
		}
	};

	namespace funcs
	{
		/**
		* ��ȡ�Ķ��ļ��б�
		*/
		Changes getChangedFiles()
		{
			Changes ret;

			auto& cache = Git::getInstance()->cach_files;
			auto& curre = Cur::getInstance()->getCurrFiles();

			auto changes = myers::diff(cache.begin(), cache.end(), curre.begin(), curre.end());

			auto a = cache.begin();
			auto b = curre.begin();

			Change change;

			for (auto it = changes.begin(); it != changes.end(); ++it)
			{
				switch (*it)
				{
				case myers::DEL:
				{
					change.iter = a;
					change.mode = DEL;
					ret.emplace_back(change);
					++a;
					break;
				}

				case myers::EQU:
				{
					// ���ж��޸�ʱ���Ƿ�ı�
					// ���ж��ļ����ݹ�ϣ�Ƿ�ı�
					FileStatu t_fstatu;
					getFileStatu(*b, t_fstatu);
					auto k1 = Git::getInstance()->hash_file[*a]->fileStat.mtime;
					if (Git::getInstance()->hash_file[*a]->fileStat.mtime != t_fstatu.mtime)
					{
						string file_text;
						readMappFile(*a, file_text);
			
						if (austin::MurmurHash3(file_text.c_str(), file_text.size()) != Git::getInstance()->hash_file[*a]->fileHash)
						{
							change.iter = a;
							change.mode = MODIFY;
							ret.emplace_back(change);
						}
					}
					++a;
					++b;
					break;
				}
					

				case myers::ADD:
				{
					change.iter = b;
					change.mode = ADD;
					ret.emplace_back(change);
					++b;
					break;
				}


				default:
					break;
				}
			}

			return ret;
		}
	}
	
};

namespace sys {


	/**
	* ��ʼ��ȫ�ֱ���
	*/
	void GIT_Start(const char* arg)
	{
		curr_dir = arg;
		curr_dir = curr_dir.substr(0, curr_dir.rfind("\\"));
		cache_path = curr_dir + CACHE_FILE;
		cache_dir = curr_dir + CACHE_DIR;

		if (_access(cache_dir.c_str(), 06))
		{
			std::cerr << "error:  not a git repository here." << std::endl;
			std::cerr << "using:  [init] to inital this repository" << std::endl;
			return;
		}

		Git::getInstance()->initial();
	}

	void GIT_Init()
	{

		if (_mkdir((curr_dir+"\\.git").c_str()) < 0 || _mkdir(cache_dir.c_str()) < 0)
			std::cerr << "fatal: creat cache files failed." << std::endl;
	}

	/*
	* 
	*/
	void GIT_Status()
	{
		auto changes = funcs::getChangedFiles();
		if (changes.size() == 0)
			std::cout << "work tree is clean" << std::endl;
		
		for (auto &changed : changes)
		{
			switch (changed.mode)
			{
			case ADD:
				std::cout << "    new :  " << *changed.iter << std::endl;
				break;

			case DEL:
				std::cout << " delete :  " << *changed.iter << std::endl;
				break;

			case MODIFY:
				std::cout << "moidify :  " << *changed.iter << std::endl;
				break;

			default:
				break;
			}
		}
	}

	void GIT_Commit()
	{
		auto changes = funcs::getChangedFiles();

		if (changes.size() == 0)
			return;

		for (auto& changed : changes)
		{
			switch(changed.mode)
			{
			case ADD:
				Git::getInstance()->addFile(*changed.iter);
				break;

			case DEL:
				Git::getInstance()->removeFile(*changed.iter);
				break;

			case MODIFY:
				Git::getInstance()->modifyFile(*changed.iter);
				break;

			default:
				break;
			};
		}

		sys::Git::getInstance()->finish();
	}

	void GIT_Reset()
	{
		string buffer;
		auto version = Git::getInstance()->ce.git_version;
		string path = curr_dir + "\\.git\\" + std::to_string(version-1);

		bool dbg = funcs::readCache(path, buffer);
		Git::getInstance()->version.shine_serial_decode(buffer);

		auto& last_files = Git::getInstance()->version.cgd_files;
		for (auto &cgd_file : last_files)
			Git::getInstance()->resetFile(cgd_file);
		
	}

};

