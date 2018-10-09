#ifndef CM_CACHE_HPP_
#define CM_CACHE_HPP_

#include <cstdint>
#include <functional>

class ReplaceFuncBase
{
public:
	ReplaceFuncBase(uint32_t nset, uint32_t nway);
	uint32_t virtual operator()(uint32_t nset);
	void virtual access(uint32_t nset, uint32_t nway);
	void virtual fill(uint32_t nset, uint32_t nway);
};

class IndexFuncBase
{
public:
	IndexFuncBase(uint32_t nset);
	uint32_t virtual operator()(uint64_t addr);
};

class CacheBase
{
	uint32_t virtual index(uint64_t addr);
	uint32_t *data;   // data array
	uint32_t *meta;   // metadat array

public:
	CacheBase(uint32_t nset, uint32_t nway,
		std::function<ReplaceFuncBase *(uint32_t, uint32_t)> replace,
		std::function<IndexFuncBase *(uint32_t)>);
	uint32_t virtual read(uint64_t addr);
	uint32_t virtual write(uint64_t addr, uint32_t id);
};

class L1CacheBase : Public CacheBase
{
public:
	L1CacheBase(uint32_t nset, uint32_t nway,
		std::function<ReplaceFuncBase *(uint32_t, uint32_t)> replace,
		std::function<IndexFuncBase *(uint32_t)>);
	uint32_t probe(uint64_t addr);
};

class LLCCacheBase : Public CacheBase
{
public:
	LLCCacheBase(uint32_t nset, uint32_t nway,
		std::function<ReplaceFuncBase *(uint32_t, uint32_t)> replace,
		std::function<IndexFuncBase *(uint32_t)>);
	uint32_t write(uint64_t addr, uint32_t id);
};

#endif
