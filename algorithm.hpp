#pragma once
#include "xgitconfig.h"
#include "serialize.hpp"

//*************************************** 定义哈希结构 ********************************//
struct HashValue
{
	HashValue(uint32_t a, uint32_t b, uint32_t c, uint32_t d) : v1(a), v2(b), v3(c), v4(d) {}
	bool operator < (const HashValue& other) const
	{
		if (v1 != other.v1)
			return v1 < other.v1;
		else if (v2 != other.v2)
			return v2 < other.v2;
		else if (v3 != other.v3)
			return v3 < other.v3;
		else if (v4 != other.v4)
			return v4 < other.v4;
		else
			throw "Hash conflict!";
	}
	HashValue& operator = (const HashValue& other) 
	{
		v1 = other.v1;
		v2 = other.v2;
		v3 = other.v3;
		v4 = other.v4;
		return *this;
	}
	HashValue(const HashValue& other) 
	{
		*this = other;
	}
	std::string toString() const
	{
		static char hexval[64];
		sprintf_s(hexval, 64, "%08x%08x%08x%08x", v1, v2, v3, v4);
		return std::string(hexval);
	}

	uint32_t v1, v2, v3, v4;

	SHINE_SERIAL(HashValue, v1, v2, v3, v4);
};

//*************************************** 定义增量结构 ********************************//
struct Diff
{
	uint32_t index;
	std::string content;
	SHINE_SERIAL(Diff, index, content);
};

struct Diffs
{
	std::vector<Diff> diffs;
	SHINE_SERIAL(Diffs, diffs);
};

//*************************************** end ******************************************//

namespace austin {
	
	//-----------------------------------------------------------------------------
	// MurmurHash3 was written by Austin Appleby, and is placed in the public
	// domain. The author hereby disclaims copyright to this source code.


	inline static uint32_t fmix(uint32_t h)
	{
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;

		return h;
	}

	inline static uint32_t getblock(const uint32_t* p, int i)
	{
		return p[i];
	}

	HashValue MurmurHash3(const void* key, const int len, uint32_t seed = 42)
	{
		const uint8_t* data = (const uint8_t*)key;
		const int nblocks = len / 16;

		uint32_t h1 = seed;
		uint32_t h2 = seed;
		uint32_t h3 = seed;
		uint32_t h4 = seed;

		uint32_t c1 = 0x239b961b;
		uint32_t c2 = 0xab0e9789;
		uint32_t c3 = 0x38b34ae5;
		uint32_t c4 = 0xa1e38b93;

		//----------
		// body

		const uint32_t* blocks = (const uint32_t*)(data + nblocks * 16);

		for (int i = -nblocks; i; i++)
		{
			uint32_t k1 = getblock(blocks, i * 4 + 0);
			uint32_t k2 = getblock(blocks, i * 4 + 1);
			uint32_t k3 = getblock(blocks, i * 4 + 2);
			uint32_t k4 = getblock(blocks, i * 4 + 3);

			k1 *= c1; k1 =  _rotl(k1, 15); k1 *= c2; h1 ^= k1;

			h1 =  _rotl(h1, 19); h1 += h2; h1 = h1 * 5 + 0x561ccd1b;

			k2 *= c2; k2 =  _rotl(k2, 16); k2 *= c3; h2 ^= k2;

			h2 =  _rotl(h2, 17); h2 += h3; h2 = h2 * 5 + 0x0bcaa747;

			k3 *= c3; k3 =  _rotl(k3, 17); k3 *= c4; h3 ^= k3;

			h3 =  _rotl(h3, 15); h3 += h4; h3 = h3 * 5 + 0x96cd1c35;

			k4 *= c4; k4 =  _rotl(k4, 18); k4 *= c1; h4 ^= k4;

			h4 =  _rotl(h4, 13); h4 += h1; h4 = h4 * 5 + 0x32ac3b17;
		}

		//----------
		// tail

		const uint8_t* tail = (const uint8_t*)(data + nblocks * 16);

		uint32_t k1 = 0;
		uint32_t k2 = 0;
		uint32_t k3 = 0;
		uint32_t k4 = 0;

		switch (len & 15)
		{
		case 15: k4 ^= tail[14] << 16;
		case 14: k4 ^= tail[13] << 8;
		case 13: k4 ^= tail[12] << 0;
			k4 *= c4; k4 =  _rotl(k4, 18); k4 *= c1; h4 ^= k4;

		case 12: k3 ^= tail[11] << 24;
		case 11: k3 ^= tail[10] << 16;
		case 10: k3 ^= tail[9] << 8;
		case  9: k3 ^= tail[8] << 0;
			k3 *= c3; k3 =  _rotl(k3, 17); k3 *= c4; h3 ^= k3;

		case  8: k2 ^= tail[7] << 24;
		case  7: k2 ^= tail[6] << 16;
		case  6: k2 ^= tail[5] << 8;
		case  5: k2 ^= tail[4] << 0;
			k2 *= c2; k2 =  _rotl(k2, 16); k2 *= c3; h2 ^= k2;

		case  4: k1 ^= tail[3] << 24;
		case  3: k1 ^= tail[2] << 16;
		case  2: k1 ^= tail[1] << 8;
		case  1: k1 ^= tail[0] << 0;
			k1 *= c1; k1 =  _rotl(k1, 15); k1 *= c2; h1 ^= k1;
		};

		//----------
		// finalization

		h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

		h1 += h2; h1 += h3; h1 += h4;
		h2 += h1; h3 += h1; h4 += h1;

		h1 = fmix(h1);
		h2 = fmix(h2);
		h3 = fmix(h3);
		h4 = fmix(h4);

		h1 += h2; h1 += h3; h1 += h4;
		h2 += h1; h3 += h1; h4 += h1;

		
		return HashValue(h1, h2, h3, h4);
	}
};

namespace myers {

	//------------------------------------------------------------------------
	// Diff Algorithm
	// See "An O(ND) Difference Algorithm and its Variations", by Eugene Myers.

	enum EditType { DEL, EQU, ADD };

	namespace
	{

		static const int NO_LINK = -1;

		struct VItem
		{
			int y;
			int tail;

			VItem(int y = 0, int tail = NO_LINK) : y(y), tail(tail) {}
		};

		struct TreeNode
		{
			EditType edit_type;
			int      prev;

			TreeNode(EditType edit_type, int prev) : edit_type(edit_type), prev(prev) {}
		};

	} // anonymous namespace

	using Iterator = Strings::const_iterator;
	using Contailer = std::vector<EditType>;

	static Contailer diff(Iterator first_a, Iterator last_a, Iterator first_b, Iterator last_b)
	{
		const int size_a = std::distance(first_a, last_a);
		const int size_b = std::distance(first_b, last_b);
		const int offset = size_a;

		std::vector<VItem> v(size_a + size_b + 1);

		std::vector<TreeNode> tree;
		int tail = NO_LINK;

		for (int d = 0; d <= size_a + size_b; ++d)
		{
			for (int k = -d; k <= d; k += 2)
			{
				if ((k < -size_a) || (size_b < k))
				{
					continue;
				}

				auto v_k = v.begin() + ( offset + k);
				auto v_kp1 = v_k + 1;
				//auto v_km1 = v_k - 1;
				auto v_km1 = v.begin();
				if (v_k != v.begin())
					v_km1 = v_k - 1;

				if (d != 0)
				{
					if (((k == -d) || (k == -size_a)) || (((k != d) && (k != size_b)) && ((v_km1->y + 1) < v_kp1->y)))
					{
						v_k->y = v_kp1->y;
						v_k->tail = tree.size();
						tree.push_back(TreeNode(DEL, v_kp1->tail));
					}
					else
					{
						v_k->y = v_km1->y + 1;
						v_k->tail = tree.size();
						tree.push_back(TreeNode(ADD, v_km1->tail));
					}
				}

				while (((v_k->y - k) < size_a) && (v_k->y < size_b) && (*(first_a + v_k->y - k) == *(first_b + v_k->y)))
				{
					TreeNode node(EQU, v_k->tail);
					v_k->tail = tree.size();
					tree.push_back(node);
					++v_k->y;
				}

				if (((v_k->y - k) >= size_a) && (v_k->y >= size_b))
				{
					Contailer ses;
					for (int i = v_k->tail; i != NO_LINK; i = tree[i].prev)
					{
						ses.push_back(tree[i].edit_type);
					}

					return Contailer(ses.rbegin(), ses.rend());
				}
			}
		}
		throw "not found";
	}

	Diffs get_diff(const Strings& stra, const Strings& strb)
	{
		Contailer ses = diff(stra.begin(), stra.end(), strb.begin(), strb.end());
		Diffs ret;
		int index = 0;

		auto a = stra.begin();
		auto b = strb.begin();

		const auto first_a = a;
		const auto first_b = b;

		Diff obj;

		for (auto it = ses.begin(); it != ses.end(); ++it)
		{
			switch (*it)
			{
			case DEL:
				//std::cout << "- " << *a;
				obj.index = index;
				obj.content = "";
				ret.diffs.push_back(obj);
				index--;
				++a;
				break;

			case EQU:
				//std::cout << "  " << *a;
				++a;
				++b;
				break;

			case ADD:
				//std::cout << "+ " << *b;
				obj.index = index;
				obj.content = *b;
				ret.diffs.push_back(obj);
				++b;
				break;

			default:
				break;
			}
			index++;
			//std::cout << "  " << index << std::endl;
		}

		return ret;
	}
};
