#pragma once
#include "xgitconfig.h"
#include "algorithm.hpp"
#include "serialize.hpp"


using std::string;

//******************************* 定义缓存结构 ********************************//
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
	bool      isDelete;

	SHINE_SERIAL(File, nameHash, fileHash, filename, fileStat, version, isDelete);
};

class Caches
{
public:
	// waring : 删除元素时迭代器会失效
	std::set<File>	files;		// 文件状态列表
	uint32_t		git_version;// 版本号
	
	SHINE_SERIAL(Caches, files, git_version);
};

class Version
{
public:
	Strings cgd_files;
	SHINE_SERIAL(Version, cgd_files);
};

//********************************* end ***********************************//
	//           .\.git\cache  .\.git\objs  .\ 

namespace sys {
	
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
		* 使用内存映射读取文件，
		*/
		bool readMappFile(const string& name, string& out)
		{
#ifdef WIN32
			// 内存映射
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
			if (mapobj == nullptr)
				return true;
			void* mapfile = MapViewOfFile(mapobj, FILE_MAP_READ, 0, 0, GetFileSize(hfile, NULL));

			out = (static_cast<char*>(mapfile));

			UnmapViewOfFile((LPVOID)mapfile);
			CloseHandle(mapobj);
			CloseHandle(hfile);
#else
			int fd = open(name.c_str(), O_RDONLY);
			if (fd < 0)
				return false;
			
			struct stat statu;

			if (fstat(fd, &statu) < 0) 
			{
				close(fd);
				return false;
			}

			void* map = mmap(NULL, statu.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
			close(fd);

			if (-1 == (int)(long)map)
				return NULL;

			out = static_cast<char*>(map);
			munmap(map, statu.st_size);
#endif
			return true;

		}
		
		/**
		* 以换行符分割字符串
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
		*  读取增量
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
		* 写增量到文件中
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
		 *  创建目录
		 */
		bool makeDir(const string& path)
		{
			return false;
		}

		/**
		 * 读取缓存文件
		 */
		bool readCache(const string& path, string& buffer)
		{
			std::ifstream in(path, std::ios::binary);
			if (!in.is_open())
			{
				std::cerr << "fatal: read cache failed." << std::endl;
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
		 * 写缓存文件
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
		 * 获取文件状态
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
			_close(fd);
			return true;
		}
	};
};

// 类定义区 // 
namespace sys 
{

	/**
	 * 单例模式
	 *		维护缓存信息
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
		* 初始化：读取缓存
		*	1. 缓存中读取ce
		*	2. 建立缓存中文件列表 cach_files，并排序
		*	3. 建立缓存中文件名 -> 文件指针映射
		*/
		bool initial()
		{
			string buffer;
			if (funcs::readCache(sys::cache_path, buffer) == false)
				return false;

			ce.shine_serial_decode(buffer);

			// 建立ce的文件列表 和文件指针映射
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
			funcs::writeCache(curr_dir + GIT_DIR + PATH_SEP + std::to_string(ce.git_version++), buffer);
			version.cgd_files.clear();
			buffer = ce.shine_serial_encode();
			return funcs::writeCache(cache_path, buffer);
		}

		/**
		 * 添加一个文件到缓存中
		 */
		bool addFile(const string& name)
		{
			string file_text;
			if (funcs::readMappFile(name, file_text) == false)
				return false;

			File file;
			if (funcs::getFileStatu(name, file.fileStat) == false)
				return false;

			// to do: how to juge it is binary file
			//if (file.fileStat.mode & S_IFMT != 0x0020000)
			//	return false;

			file.version = 0;
			file.filename = name;
			file.fileHash = austin::MurmurHash3(file_text.c_str(), file_text.size());
			file.nameHash = austin::MurmurHash3(name.c_str(), name.size());
			file.isDelete = false;

			cach_files.emplace_back(name);
			hash_file[name] = ce.files.insert(file).first;
			std::sort(cach_files.begin(), cach_files.end());

			version.cgd_files.emplace_back(name);

			string write_dir = cache_dir + PATH_SEP + file.nameHash.toString();
			
			if (_mkdir(write_dir.c_str()) < 0)
				return false;

			return funcs::writeCache(write_dir + PATH_SEP + file.nameHash.toString(), file_text);
		}

		/**
		 * 从缓存中删除一个文件
		 */
		bool removeFile(const string& name)
		{
			auto const_p_file = hash_file[name];
			auto nam_hash = austin::MurmurHash3(name.c_str(), name.size());

			string incer_path = cache_dir + PATH_SEP + nam_hash.toString() + PATH_SEP + std::to_string(const_p_file->version - 1);
			string write_path = cache_dir + PATH_SEP + nam_hash.toString() + PATH_SEP + std::to_string(const_p_file->version);
			string incer_buff;

			if (const_p_file->version == 0)
				funcs::writeCache(write_path, incer_buff);
			else
			{
				funcs::readCache(incer_path, incer_buff);
				funcs::writeCache(write_path, incer_buff);
			}

			// 改文件状态
			auto pfile = const_cast<File*>(&(*const_p_file));
			pfile->version++;
			pfile->isDelete = true;

			version.cgd_files.emplace_back(name);

			return true;
		}

		/**
		 * 修改缓存中的文件
		 */
		bool modifyFile(const string& name)
		{
			auto const_pfile = hash_file[name];

			// 获取原始版本
			string  origin_path = cache_dir + PATH_SEP + const_pfile->nameHash.toString() + PATH_SEP + const_pfile->nameHash.toString();
			string  origin_text; funcs::readMappFile(origin_path, origin_text);
			Strings origin_lins; funcs::splitString(origin_text, origin_lins);

			// 获取当前版本
			string	curent_text; funcs::readMappFile(name, curent_text);
			Strings curent_lins; funcs::splitString(curent_text, curent_lins);

			// 写增量
			string incer_path = cache_dir + PATH_SEP + const_pfile->nameHash.toString() + PATH_SEP + std::to_string(const_pfile->version);
			auto diffs = myers::getDiffs(origin_lins, curent_lins);
			funcs::writeDiffsTo(incer_path, diffs);

			// 改文件状态
			auto pfile = const_cast<File*>(&(*const_pfile));
			pfile->version++;
			funcs::getFileStatu(name, pfile->fileStat);
			funcs::readMappFile(name, curent_text);
			pfile->fileHash = austin::MurmurHash3(curent_text.c_str(), curent_text.size());

			version.cgd_files.emplace_back(name);

			return true;
		}

		/**
		 * 恢复文件的上一版本
		 */
		bool resetFile(const string& name)
		{
			auto p_file = hash_file[name];

			auto origi_path = cache_dir + PATH_SEP + p_file->nameHash.toString() + PATH_SEP + p_file->nameHash.toString();
			auto incer_path = cache_dir + PATH_SEP + p_file->nameHash.toString() + PATH_SEP + std::to_string(p_file->version-2);
			string origin_text, incer_buff;
			funcs::readMappFile(origi_path, origin_text);

			if (p_file->version == 1)
				return funcs::writeCache(name, origin_text);
			if (p_file->version <= 0)
			{
				int dbg = remove(name.c_str());
				return dbg;
			}
			
			Strings origin_lines;
			funcs::splitString(origin_text, origin_lines);

			Diffs diffs;
			funcs::readDiffsForm(incer_path, diffs);

			myers::merageDiff(origin_lines, diffs);
			
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
		/* 文件状态列表 */
		Caches ce;

		/* 缓存中： 文件名列表 */
		Strings cach_files;

		/* 缓存中： 文件名 -> 文件状态指针 */
		std::map<string, std::set<File>::iterator> hash_file;

		/* 此次改动的文件列表 */
		Version version;
	};

	/**
	 * 单例模式
	 * 当前目录管理类
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
		/* 目录下： 文件名列表 */
		Strings curr_files;

#ifdef WIN32

		// x86与x64不兼容
#ifdef _WIN64
#define xfinddata	__finddata64_t
#define xfindfirst	_findfirst64
#define xfindnext	_findnext64
#define xfindhandl	__int64
#else
#define xfinddata	_finddata_t
#define xfindfirst	_findfirst
#define xfindnext	_findnext
#define xfindhandl	__int32
#endif
		/* 获取目录下所有文件列表 */
		void getFilesAll(string path, Strings& files)
		{
			//文件句柄
			xfindhandl  hFile = 0;
			//文件信息
			struct xfinddata fileinfo;
			string p;
			if ((hFile = xfindfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1)
			{
				do
				{
					if ((fileinfo.attrib & _A_SUBDIR))
					{
						if (fileinfo.name[0] != '.')
						{
							//files.push_back(p.assign(path).append("\\").append(fileinfo.name) );
							getFilesAll(p.assign(path).append("\\").append(fileinfo.name), files);
						}
					}
					else
					{
						files.push_back(p.assign(path).append("\\").append(fileinfo.name));
					}
						

				} while (xfindnext(hFile, &fileinfo) == 0);
				_findclose(hFile);
			}
		}
#else
		bool getFilesAll(string path, Strings& out)
		{
			DIR *dir = opendir(path.c_str());
			if (dir == NULL)
				return false;

			struct dirent *ptr;
			struct stat  statu;
			static string curr;

			while((ptr = readdir(dir)) != NULL)
			{
				if (ptr->d_name[0] == '.')
					continue;

				curr.assign(path).append("/").append(ptr->d_name);
				stat(curr.c_str(), &statu);
				
				if (S_ISDIR(statu.st_mode))
					getFilesAll(curr, out);

				else if (S_ISREG(statu.st_mode))
					out.emplace_back(curr);
			}

			closedir(dir);
			std::sort(out.begin(), out.end());

		}
#endif
	};

	namespace funcs
	{
		/**
		* 获取改动文件列表
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
					if (!Git::getInstance()->hash_file[*a]->isDelete)
					{
						change.iter = a;
						change.mode = DEL;
						ret.emplace_back(change);
					}
					++a;
					break;
				}

				case myers::EQU:
				{
					// 判断修改时间是否改变
					FileStatu t_fstatu;
					getFileStatu(*b, t_fstatu);
					if (Git::getInstance()->hash_file[*a]->fileStat.mtime != t_fstatu.mtime)
					{
						// 判断文件内容哈希是否改变
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
	* 初始化全局变量
	*/
	void GIT_Start(const char* arg)
	{
		curr_dir = arg;
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

	/**
	 * 创建仓库
	 */
	void GIT_Init()
	{

		if (_mkdir((curr_dir + GIT_DIR).c_str()) < 0 || _mkdir(cache_dir.c_str()) < 0)
			std::cerr << "fatal: creat cache files failed." << std::endl;
	}

	/**
	 * 获取状态
	 */
	void GIT_Status()
	{
		auto changes = funcs::getChangedFiles();
		
		if (changes.size() == 0)
			std::cout << "the current version is " << Git::getInstance()->ce.git_version
			<< ". work tree is clean" << std::endl;
		
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

	/**
	 * 提交版本
	 */
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

	/**
	 * 回退版本
	 */
	void GIT_Reset()
	{
		string buffer;
		auto version = Git::getInstance()->ce.git_version--;
		string path = curr_dir + GIT_DIR + PATH_SEP + std::to_string(version-1);

		bool dbg = funcs::readCache(path, buffer);
		Git::getInstance()->version.shine_serial_decode(buffer);

		auto& last_files = Git::getInstance()->version.cgd_files;
		for (auto& cgd_file : last_files)
		{
			Git::getInstance()->resetFile(cgd_file);
		}
		
	}

};

