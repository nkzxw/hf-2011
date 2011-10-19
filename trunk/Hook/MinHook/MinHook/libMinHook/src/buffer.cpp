/* 
 *  MinHook - Minimalistic API Hook Library	
 *  Copyright (C) 2009 Tsuda Kageyu. All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cassert>
#include <vector>
#include <algorithm>
#include <boost/foreach.hpp>
#include <Windows.h>

#include "buffer.h"

namespace MinHook { namespace
{
	struct MEMORY_BLOCK
	{
		void*	pAddress;
		DWORD	protect;
		size_t	usedSize;
		size_t	fixedSize;
	};

	template <typename T>
	bool operator <(const MEMORY_BLOCK& lhs, const T& rhs);
	template <typename T>
	bool operator <(const T& lhs, const MEMORY_BLOCK& rhs);
	bool operator <(const MEMORY_BLOCK& lhs, const MEMORY_BLOCK& rhs);

	void*			AllocateBuffer(void* const pOrigin, DWORD protect, size_t size);
	MEMORY_BLOCK*	GetMemoryBlock(void* const pOrigin, DWORD protect, size_t capacity);

	const size_t BlockSize = 0x10000;

#if defined _M_X64
	intptr_t gMinAddress;
	intptr_t gMaxAddress;
#endif
	std::vector<MEMORY_BLOCK> gMemoryBlocks;
}}

namespace MinHook 
{
	void InitializeBuffer()
	{
#if defined _M_X64
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		
		gMinAddress = reinterpret_cast<intptr_t>(si.lpMinimumApplicationAddress);
		gMaxAddress = reinterpret_cast<intptr_t>(si.lpMaximumApplicationAddress);
#endif
	}

	void UninitializeBuffer()
	{
		BOOST_FOREACH (MEMORY_BLOCK& block, gMemoryBlocks)
		{
			VirtualFree(block.pAddress, 0, MEM_RELEASE);
		}

		std::vector<MEMORY_BLOCK> v;
		gMemoryBlocks.swap(v);
	}

	void* AllocateCodeBuffer(void* const pOrigin, size_t size)
	{
		assert(("AllocateBuffer", (size > 0)));

		return AllocateBuffer(pOrigin, PAGE_EXECUTE_READ, size);
	}

	void* AllocateDataBuffer(void* const pOrigin, size_t size)
	{
		assert(("AllocateBuffer", (size > 0)));

		return AllocateBuffer(pOrigin, PAGE_READONLY, size);
	}

	void RollbackBuffer()
	{
		BOOST_FOREACH (MEMORY_BLOCK& block, gMemoryBlocks)
		{
			block.usedSize = block.fixedSize;
		}
	}

	void CommitBuffer()
	{
		BOOST_FOREACH (MEMORY_BLOCK& block, gMemoryBlocks)
		{
			if (block.usedSize == block.fixedSize)
			{
				continue;
			}

			void* pBuffer = reinterpret_cast<char*>(block.pAddress) + block.fixedSize;
			size_t size = block.usedSize - block.fixedSize;
			DWORD op;
			VirtualProtect(pBuffer, size, block.protect, &op);
		}
	}
}

namespace MinHook { namespace
{
	void* AllocateBuffer(void* const pOrigin, DWORD protect, size_t size)
	{
		assert(("AllocateBuffer", (protect == PAGE_EXECUTE_READ || protect == PAGE_READONLY)));
		assert(("AllocateBuffer", (size > 0)));

		// �A���C�����g���E�ɐ؂�グ
		size = (size + TYPE_ALIGNMENT(void*) - 1) & ~(TYPE_ALIGNMENT(void*) - 1);

		MEMORY_BLOCK* pBlock = GetMemoryBlock(pOrigin, protect, size);
		if (pBlock == NULL)
		{
			return NULL;
		}

		void* pBuffer = reinterpret_cast<char*>(pBlock->pAddress) + pBlock->usedSize;
		if (VirtualAlloc(pBuffer, size, MEM_COMMIT, pBlock->protect) == NULL)
		{
			return NULL;
		}

		DWORD oldProtect;
		// PAGE_EXECUTE_READ -> PAGE_EXECUTE_READWRITE, PAGE_READONLY -> PAGE_READWRITE
		if (!VirtualProtect(pBuffer, size, (pBlock->protect << 1), &oldProtect))
		{
			return NULL;
		}

		pBlock->usedSize += size;
		return pBuffer;
	}

	MEMORY_BLOCK* GetMemoryBlock(void* const pOrigin, DWORD protect, size_t capacity)
	{
		assert(("GetMemoryBlock", (protect == PAGE_EXECUTE_READ || protect == PAGE_READONLY)));
		assert(("GetMemoryBlock", (capacity > 0)));

		typedef std::vector<MEMORY_BLOCK>::iterator mb_iter;

#if defined _M_X64
		intptr_t minAddr = gMinAddress;
		intptr_t maxAddr = gMaxAddress; 
		if (pOrigin != NULL)
		{
			// pOrigin �} 512MB �͈̔� 
			minAddr = std::max<intptr_t>(minAddr, reinterpret_cast<intptr_t>(pOrigin) - 0x20000000);
			maxAddr = std::min<intptr_t>(maxAddr, reinterpret_cast<intptr_t>(pOrigin) + 0x20000000);
		}
#endif

		// ���łɓo�^�ς݂̗̈�̒�����g�p�\�Ȃ��̂�������΁A�����Ԃ�
		MEMORY_BLOCK* pBlock = NULL;
		{
			mb_iter ib = gMemoryBlocks.begin();
			mb_iter ie = gMemoryBlocks.end();
#if defined _M_X64
			if (pOrigin != NULL)
			{
				// �����O�ɃA�h���X�͈͂ōi�荞��
				ib = std::lower_bound(ib, ie, minAddr);
				ie = std::lower_bound(ib, ie, maxAddr);
			}
#endif
			for (mb_iter i = ib; i != ie; ++i)
			{
				if (i->protect == protect && i->usedSize + capacity <= BlockSize)
				{
					return &(*i);
				}
			}
		}

		// ������Ȃ���΁A�V���ȃA�h���X�̈���m��
		void* pAlloc = NULL;
#if defined _M_X64
		if (pOrigin != NULL)
		{
			// �����͈͂̒��S����O���֋󂫗̈��T���Ă���
			intptr_t min = minAddr / BlockSize;
			intptr_t max = maxAddr / BlockSize;
			int rel = 0;
			MEMORY_BASIC_INFORMATION mi = { 0 };
			for (int i = 0; i < (max - min + 1); ++i)
			{
				rel = -rel + (i & 1);
				void* pQuery = reinterpret_cast<void*>(((min + max) / 2 + rel) * BlockSize);
				VirtualQuery(pQuery, &mi, sizeof(mi));
				if (mi.State == MEM_FREE)
				{
					pAlloc = VirtualAlloc(pQuery, BlockSize, MEM_RESERVE, protect);
					if (pAlloc != NULL)
					{
						break;
					}
				}
			}
		}
		else
#endif		// X86���[�h�ł́A�A�h���X�͖��ɂȂ�Ȃ�
		{
			pAlloc = VirtualAlloc(NULL, BlockSize, MEM_RESERVE, protect);
		}

		if (pAlloc != NULL)
		{
			MEMORY_BLOCK block = { 0 };
			block.pAddress = pAlloc;
			block.protect = protect;

#if defined _M_X64
			mb_iter i = std::lower_bound(gMemoryBlocks.begin(), gMemoryBlocks.end(), pAlloc);
#elif defined _M_IX86
			mb_iter i = gMemoryBlocks.begin();
#endif
			i = gMemoryBlocks.insert(i, block);

			return &(*i);
		}

		return NULL;
	}

	template <typename T>
	bool operator <(const MEMORY_BLOCK& lhs, const T& rhs) 
	{ 
		return lhs.pAddress < reinterpret_cast<void*>(rhs); 
	}

	template <typename T>
	bool operator <(const T& lhs, const MEMORY_BLOCK& rhs) 
	{ 
		return reinterpret_cast<void*>(lhs) < rhs.pAddress; 
	}

	bool operator <(const MEMORY_BLOCK& lhs, const MEMORY_BLOCK& rhs)
	{ 
		return lhs.pAddress < rhs.pAddress;
	}
}}