#ifndef RQ_API_H
#define RQ_API_H
#include <stddef.h>
#include <stdint.h>

#ifdef RQAPI_BUILD
#define RQAPI __attribute__((visibility("default")))
#else
#define RQAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Intermediate Block API functions

RQAPI
int RqInterGetMemSizes(int nMaxK,
		       int nMaxExtra,
		       size_t* pInterWorkMemSize,
		       size_t* pInterProgMemSize,
		       size_t* pInterSymNum);

struct RqInterWorkMem_;

typedef struct RqInterWorkMem_ RqInterWorkMem;

RQAPI
int RqInterInit(int nK,
		int nMaxExtra,
		RqInterWorkMem* pInterWorkMem,
		size_t nInterWorkMemSize);

RQAPI
int RqInterAddIds(RqInterWorkMem* pInterWorkMem,
		  int32_t nInSymIdBeg,
		  int32_t nInSymIdAdd);

struct RqInterProgram_;

typedef struct RqInterProgram_ RqInterProgram;

RQAPI
int RqInterCompile(RqInterWorkMem* pInterWorkMem,
		   RqInterProgram* pInterProgMem,
		   size_t nInterProgMemSize);

RQAPI
int RqInterExecute(const RqInterProgram* pcInterProgMem,
		   size_t nSymSize,
		   const void* pcInSymMem,
		   size_t nInSymMemSize,
		   void* pInterSymMem,
		   size_t nInterSymMemSize);

// Output Symbol API functions
struct RqOutWorkMem_;
typedef struct RqOutWorkMem_ RqOutWorkMem;

RQAPI
int RqOutGetMemSizes(	int nOutSymNum,
			size_t* pOutWorkMemSize,
			size_t* pOutProgMemSize);

RQAPI
int RqOutInit(	int nK,
		RqOutWorkMem* pOutWorkMem,
		size_t nOutWorkMemSize);

RQAPI
int RqOutAddIds(	RqOutWorkMem* pOutWorkMem,
			int32_t nOutSymIdBeg,
			int32_t nOutSymIdAdd);

struct RqOutProgram_;
typedef struct RqOutProgram_ RqOutProgram;

RQAPI
int RqOutCompile(       RqOutWorkMem* pOutWorkMem,
			RqOutProgram* pOutProgMem,
			size_t nOutProgMemSize);

RQAPI
int RqOutExecute(	const RqOutProgram* pcOutProgMem,
			size_t nSymSize,
			const void* pcInterSymMem,
			void* pOutSymMem,
			size_t nOutSymMemSize);


// Constants

#define RQ_MAX_K			56403
#define RQ_DEFAULT_MAX_EXTRA		30

// Error Codes
#define RQ_ERR_ENOMEM			(-1)
#define RQ_ERR_EDOM			(-2)
#define RQ_ERR_MAX_IDS_REACHED		(-3)
#define RQ_ERR_INSUFF_IDS		(-4)

#ifdef __cplusplus
}
#endif

#endif /* RQ_API_H */
