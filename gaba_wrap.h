
/**
 * @file gaba_wrap.c
 *
 * @brief wrapper API implementation of the GABA library
 * gaba.c must be compiled with the following combinations:
 * {-DBW=16, -DBW=32} x {-DMODEL=LINEAR, -DMODEL=AFFINE}.
 *
 * @author Hajime Suzuki
 * @date 2016/6/1
 * @license Apache v2
 */

#ifndef _GABA_WRAP_H_INCLUDED
#define _GABA_WRAP_H_INCLUDED

/* import unittest */
#ifndef UNITTEST_UNIQUE_ID
#  define UNITTEST_UNIQUE_ID		33
#endif
#include  "unittest.h"


#include <stdint.h>							/* uint32_t, uint64_t, ... */
#include "gaba.h"
#include "sassert.h"
#include "arch/arch.h"


/* bandwidth-related macros (see gaba.c) */
#define DP_CTX_MAX					( 2 )
#define _dp_ctx_index(_bw)			( DP_CTX_MAX - ((_bw)>>4) )
_static_assert(_dp_ctx_index(32) == 0);		/* assume 32-cell has the smallest index */


/* add namespace for arch wrapper (see main.c) */
#ifndef _import
#  ifdef NAMESPACE
#    define _import_cat(x, y)		x##_##y
#    define _import_cat2(x, y)		_import_cat(x, y)
#    define _import(_base)			_import_cat2(NAMESPACE, _base)
#  else
#    define _import(_base)			_base
#  endif
#endif


/* gap penalty model (linear or affine) */
#define LINEAR 						1
#define AFFINE						2


/**
 * @struct gaba_api_s
 *
 * @brief a set of pointers to GABA API
 */
struct gaba_api_s {
	/* fill-in */
	gaba_fill_t *(*dp_fill_root)(
		gaba_dp_t *self,
		gaba_section_t const *a,
		uint32_t apos,
		gaba_section_t const *b,
		uint32_t bpos);
	gaba_fill_t *(*dp_fill)(
		gaba_dp_t *self,
		gaba_fill_t const *prev_sec,
		gaba_section_t const *a,
		gaba_section_t const *b);
	gaba_pos_pair_t (*dp_search_max)(
		gaba_dp_t *self,
		gaba_fill_t const *sec);

	/* trace */
	gaba_alignment_t *(*dp_trace)(
		gaba_dp_t *self,
		gaba_fill_t const *fw_tail,
		gaba_fill_t const *rv_tail,
		gaba_trace_params_t const *params);
};
_static_assert(sizeof(struct gaba_api_s) == 4 * sizeof(void *));		/* must be consistent to gaba_opaque_s */
#define _api(_dp)				( (struct gaba_api_s const *)(_dp) )
#define _api_array(_ctx)		( (struct gaba_api_s const (*)[DP_CTX_MAX])(_ctx) )

/**
 * @struct redefinition of gaba_dp_context_s
 */
struct gaba_dp_context_s {
	struct gaba_api_s bw[2];	/** (64) [0] for 32-cell, [1] for 16-cell */
};


/* forward declarations */
#define _decl_cat3_2(a, b, c)	a##_##b##_##c
#define _decl_cat3(a, b, c)		_decl_cat3_2(a, b, c)
#define _decl(ret_t, _base, ...) \
	ret_t _import(_decl_cat3(_base, linear, 16))(__VA_ARGS__); \
	ret_t _import(_decl_cat3(_base, affine, 16))(__VA_ARGS__); \
	ret_t _import(_decl_cat3(_base, linear, 32))(__VA_ARGS__); \
	ret_t _import(_decl_cat3(_base, affine, 32))(__VA_ARGS__);

_decl(gaba_t *, gaba_init, gaba_params_t const *params);
_decl(void, gaba_clean, gaba_t *ctx);
_decl(struct gaba_dp_context_s *, gaba_dp_init, gaba_t const *ctx, uint8_t const *alim, uint8_t const *blim);
_decl(void, gaba_dp_flush, gaba_dp_t *self, uint8_t const *alim, uint8_t const *blim);
_decl(gaba_stack_t const *, gaba_dp_save_stack, gaba_dp_t *self);
_decl(void, gaba_dp_flush_stack, gaba_dp_t *self, gaba_stack_t const *stack);
_decl(void, gaba_dp_clean, gaba_dp_t *self);
_decl(gaba_fill_t *, gaba_dp_fill_root, gaba_dp_t *self, gaba_section_t const *a, uint32_t apos, gaba_section_t const *b, uint32_t bpos);
_decl(gaba_fill_t *, gaba_dp_fill, gaba_dp_t *self, gaba_fill_t const *prev_sec, gaba_section_t const *a, gaba_section_t const *b);
_decl(gaba_fill_t *, gaba_dp_merge, gaba_dp_t *self, gaba_fill_t const *sec_list, uint64_t sec_list_len);
_decl(gaba_pos_pair_t, gaba_dp_search_max, gaba_dp_t *self, gaba_fill_t const *sec);
_decl(gaba_alignment_t *, gaba_dp_trace, gaba_dp_t *self, gaba_fill_t const *fw_tail, gaba_fill_t const *rv_tail, gaba_trace_params_t const *params);
_decl(gaba_alignment_t *, gaba_dp_recombine, gaba_dp_t *self, gaba_alignment_t *x, uint32_t xsid, gaba_alignment_t *y, uint32_t ysid);
_decl(void, gaba_dp_res_free, gaba_alignment_t *res);
_decl(int64_t, gaba_dp_print_cigar_forward, gaba_dp_printer_t printer, void *fp, uint32_t const *path, uint32_t offset, uint32_t len);
_decl(int64_t, gaba_dp_print_cigar_reverse, gaba_dp_printer_t printer, void *fp, uint32_t const *path, uint32_t offset, uint32_t len);
_decl(int64_t, gaba_dp_dump_cigar_forward, char *buf, uint64_t buf_size, uint32_t const *path, uint32_t offset, uint32_t len);
_decl(int64_t, gaba_dp_dump_cigar_reverse, char *buf, uint64_t buf_size, uint32_t const *path, uint32_t offset, uint32_t len);

#undef _decl

/* function table */
static
struct gaba_api_s const api_table[][DP_CTX_MAX] __attribute__(( aligned(16) )) = {
	#define _table_elems(_model, _bw) { \
		.dp_fill_root = _import(_decl_cat3(gaba_dp_fill_root, _model, _bw)), \
		.dp_fill = _import(_decl_cat3(gaba_dp_fill, _model, _bw)), \
		.dp_search_max = _import(_decl_cat3(gaba_dp_search_max, _model, _bw)), \
		.dp_trace = _import(_decl_cat3(gaba_dp_trace, _model, _bw)) \
	}

	[LINEAR] = {
		[_dp_ctx_index(16)] = _table_elems(linear, 16),
		[_dp_ctx_index(32)] = _table_elems(linear, 32)
	},
	[AFFINE] = {
		[_dp_ctx_index(16)] = _table_elems(affine, 16),
		[_dp_ctx_index(32)] = _table_elems(affine, 32)
	}

	#undef _table_elems
};

/**
 * @fn gaba_init_get_index
 */
static inline
int64_t gaba_init_get_index(
	struct gaba_params_s const *params)
{
	if(params == NULL) {
		return(AFFINE);
	}

	if(params->gi != 0) {
		return(AFFINE);
	}
	return(LINEAR);
}

/**
 * @fn gaba_set_api
 */
static inline
void *gaba_set_api(
	void *ctx,
	struct gaba_api_s const (*api)[DP_CTX_MAX])
{
	if(ctx == NULL) { return(NULL); }
	struct gaba_api_s *dst = (struct gaba_api_s *)ctx;

	for(uint64_t i = 0; i < DP_CTX_MAX; i++) {
		_memcpy_blk_aa(&dst[i], &((struct gaba_api_s const *)api)[i], sizeof(struct gaba_api_s));
	}
	return((void *)dst);
}

/**
 * @fn gaba_init
 */
static inline
gaba_t *gaba_init(
	gaba_params_t const *params)
{
	if(params == NULL) {
		return(NULL);
	}

	uint64_t idx = gaba_init_get_index(params);
	struct gaba_api_s const (*api)[DP_CTX_MAX] = &api_table[idx];

	/* create context */
	gaba_params_t p = *params;
	gaba_t *ctx = NULL;
	ctx = ((idx == LINEAR) ? gaba_init_linear_16 : gaba_init_affine_16)(&p);

	/* init 32-cell wide root block */
	p.reserved = (void *)ctx;
	ctx = ((idx == LINEAR) ? gaba_init_linear_32 : gaba_init_affine_32)(&p);
	return((gaba_t *)gaba_set_api((void *)ctx, api));
}

/**
 * @fn gaba_clean
 */
static inline
void gaba_clean(
	gaba_t *ctx)
{
	// _api(ctx)->clean(ctx);
	gaba_clean_linear_32(ctx);
	return;
}

/**
 * @fn gaba_dp_init
 */
static inline
struct gaba_dp_context_s *gaba_dp_init(
	gaba_t const *ctx,
	uint8_t const *alim,
	uint8_t const *blim)
{
	return((gaba_dp_t *)gaba_set_api((void *)_import(gaba_dp_init_linear_32)(ctx, alim, blim), _api_array(ctx)));
}

/**
 * @fn gaba_dp_flush
 */
static inline
void gaba_dp_flush(
	gaba_dp_t *self,
	uint8_t const *alim,
	uint8_t const *blim)
{
	_import(gaba_dp_flush_linear_32)(self, alim, blim);
	return;
}

/**
 * @fn gaba_dp_save_stack
 */
static inline
gaba_stack_t const *gaba_dp_save_stack(
	gaba_dp_t *self)
{
	return(_import(gaba_dp_save_stack_linear_32)(self));
}

/**
 * @fn gaba_dp_flush_stack
 */
static inline
void gaba_dp_flush_stack(
	gaba_dp_t *self,
	gaba_stack_t const *stack)
{
	_import(gaba_dp_flush_stack_linear_32)(self, stack);
	return;
}

/**
 * @fn gaba_dp_clean
 */
static inline
void gaba_dp_clean(
	gaba_dp_t *self)
{
	_import(gaba_dp_clean_linear_32)(self);
	return;
}

/**
 * @fn gaba_dp_fill_root
 */
static inline
gaba_fill_t *gaba_dp_fill_root(
	gaba_dp_t *self,
	gaba_section_t const *a,
	uint32_t apos,
	gaba_section_t const *b,
	uint32_t bpos)
{
	return(_api(self)->dp_fill_root(self, a, apos, b, bpos));
}

/**
 * @fn gaba_dp_fill
 * @brief fill dp matrix inside section pairs
 */
static inline
gaba_fill_t *gaba_dp_fill(
	gaba_dp_t *self,
	gaba_fill_t const *prev_sec,
	gaba_section_t const *a,
	gaba_section_t const *b)
{
	return(_api(self)->dp_fill(self, prev_sec, a, b));
}

/**
 * @fn gaba_dp_merge
 */
static inline
gaba_fill_t *gaba_dp_merge(
	gaba_dp_t *self,
	gaba_fill_t const *sec_list,
	uint64_t sec_list_len)
{
	// return(_api(self)->dp_merge(self, sec_list, sec_list_len));
	return(NULL);		/* not implemented yet */
}

/**
 * @fn gaba_dp_search_max
 */
static inline
gaba_pos_pair_t gaba_dp_search_max(
	gaba_dp_t *self,
	gaba_fill_t const *sec)
{
	return(_api(self)->dp_search_max(self, sec));
}

/**
 * @fn gaba_dp_trace
 */
static inline
gaba_alignment_t *gaba_dp_trace(
	gaba_dp_t *self,
	gaba_fill_t const *fw_tail,
	gaba_fill_t const *rv_tail,
	gaba_trace_params_t const *params)
{
	return(_api(self)->dp_trace(self, fw_tail, rv_tail, params));
}

/**
 * @fn gaba_dp_recombine
 */
static inline
gaba_alignment_t *gaba_dp_recombine(
	gaba_dp_t *self,
	gaba_alignment_t *x,
	uint32_t xsid,
	gaba_alignment_t *y,
	uint32_t ysid)
{
	return(_import(gaba_dp_recombine_linear_32)(self, x, xsid, y, ysid));
}

/**
 * @fn gaba_dp_res_free
 */
static inline
void gaba_dp_res_free(
	gaba_alignment_t *res)
{
	_import(gaba_dp_res_free_linear_32)(res);
	return;
}


/**
 * @fn gaba_dp_print_cigar_forward
 */
static inline
uint64_t gaba_dp_print_cigar_forward(
	gaba_dp_printer_t printer,
	void *fp,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	return(_import(gaba_dp_print_cigar_forward_linear_32)(printer, fp, path, offset, len));
}

/**
 * @fn gaba_dp_print_cigar_reverse
 */
static inline
uint64_t gaba_dp_print_cigar_reverse(
	gaba_dp_printer_t printer,
	void *fp,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	return(_import(gaba_dp_print_cigar_reverse_linear_32)(printer, fp, path, offset, len));
}

/**
 * @fn gaba_dp_dump_cigar_forward
 */
static inline
uint64_t gaba_dp_dump_cigar_forward(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	return(_import(gaba_dp_dump_cigar_forward_linear_32)(buf, buf_size, path, offset, len));
}

/**
 * @fn gaba_dp_dump_cigar_reverse
 */
static inline
uint64_t gaba_dp_dump_cigar_reverse(
	char *buf,
	uint64_t buf_size,
	uint32_t const *path,
	uint32_t offset,
	uint32_t len)
{
	return(_import(gaba_dp_dump_cigar_reverse_linear_32)(buf, buf_size, path, offset, len));
}


/* unittests */
#if UNITTEST != 0

/**
 * @struct unittest_seqs_s
 * @brief sequence container
 */
struct unittest_seqs_s {
	char const *a;
	char const *b;
};
#define unittest_seq_pair(_a, _b) ( \
	(void *)&((struct unittest_seqs_s const){ \
		.a = _a "GGGGGGGGGGGGGGGGGGGG", \
		.b = _b "CCCCCCCCCCCCCCCCCCCC" \
	}) \
)

/**
 * @struct unittest_sections_s
 * @brief section container
 */
struct unittest_sections_s {
	uint8_t const *a, *b;
	uint8_t const *alim, *blim;
	uint64_t alen, blen;
	struct gaba_section_s afsec, aftail, bfsec, bftail;
	struct gaba_section_s arsec, artail, brsec, brtail;
};

/**
 * @fn unittest_encode_base
 * @brief mapping IUPAC amb. to 2bit / 4bit encoding
 */
static inline
uint8_t unittest_encode_base(
	char c)
{
	/* convert to upper case and subtract offset by 0x40 */
	#define _b(x)	( (x) & 0x1f )

	/* conversion tables */
	#if BIT == 2
		enum bases { A = 0x00, C = 0x01, G = 0x02, T = 0x03 };
		static uint8_t const table[] = {
			[_b('A')] = A,
			[_b('C')] = C,
			[_b('G')] = G,
			[_b('T')] = T,
			[_b('U')] = T,
			[_b('N')] = A,		/* treat 'N' as 'A' */
			[_b('_')] = 0		/* sentinel */
		};
	#else /* BIT == 4 */
		enum bases { A = 0x01, C = 0x02, G = 0x04, T = 0x08 };
		static uint8_t const table[] = {
			[_b('A')] = A,
			[_b('C')] = C,
			[_b('G')] = G,
			[_b('T')] = T,
			[_b('U')] = T,
			[_b('R')] = A | G,
			[_b('Y')] = C | T,
			[_b('S')] = G | C,
			[_b('W')] = A | T,
			[_b('K')] = G | T,
			[_b('M')] = A | C,
			[_b('B')] = C | G | T,
			[_b('D')] = A | G | T,
			[_b('H')] = A | C | T,
			[_b('V')] = A | C | G,
			[_b('N')] = 0,		/* treat 'N' as a gap */
			[_b('_')] = 0		/* sentinel */
		};
	#endif /* BIT */
	return(table[_b((uint8_t)c)]);

	#undef _b
}

/**
 * @fn unittest_build_seqs
 * @brief build seq_pair and sections
 */
static
void *unittest_build_seqs(void *params)
{
	struct unittest_seqs_s const *seqs = (struct unittest_seqs_s const *)params;
	char const *a = seqs->a;
	char const *b = seqs->b;

	uint64_t atot = strlen(a);
	uint64_t btot = strlen(b);
	uint32_t alen = atot - 20;
	uint32_t blen = btot - 20;

	uint64_t margin = 64;
	struct unittest_sections_s *sec = malloc(
		sizeof(struct unittest_sections_s) + (atot + 1) + (btot + 1) + margin);

	/* copy sequences */
	uint8_t *ca = (uint8_t *)(sec + 1);
	uint8_t *cb = ca + atot + 1;
	uint8_t *cm = cb + btot + 1;

	for(uint64_t i = 0; i < atot; i++) {
		ca[i] = unittest_encode_base(a[i]);
	}
	for(uint64_t i = 0; i < btot; i++) {
		cb[i] = unittest_encode_base(b[i]);
	}
	ca[atot] = cb[btot] = '\0';
	memset(cm, 0, margin);

	/* build context */
	uint8_t const *const alim = (void const *)0x800000000000;
	uint8_t const *const blim = (void const *)0x800000000000;
	*sec = (struct unittest_sections_s){
		.a = ca,
		.b = cb,
		.alim = alim,
		.blim = blim,
		.alen = atot,
		.blen = btot,
		
		/* forward */
		.afsec = gaba_build_section(0, ca, alen),
		.aftail = gaba_build_section(2, ca + alen, 20),
		.bfsec = gaba_build_section(4, cb, blen),
		.bftail = gaba_build_section(6, cb + blen, 20),

		/* reverse */
		.arsec = gaba_build_section(1, gaba_rev(ca + alen, alim), alen),
		.artail = gaba_build_section(3, gaba_rev(ca + atot, alim), 20),
		.brsec = gaba_build_section(5, gaba_rev(cb + blen, blim), blen),
		.brtail = gaba_build_section(7, gaba_rev(cb + btot, blim), 20)
	};
	return((void *)sec);
}

/**
 * @fn unittest_clean_seqs
 */
static
void unittest_clean_seqs(void *ctx)
{
	free(ctx);
}

/**
 * @macro with_seq_pair
 * @brief constructs .param, .init, and .clean parameters
 */
#define with_seq_pair(_a, _b) \
	.params = unittest_seq_pair(_a, _b), \
	.init = unittest_build_seqs, \
	.clean = unittest_clean_seqs

#define omajinai() \
	struct unittest_sections_s const *s = (struct unittest_sections_s const *)ctx;


unittest_config(
	.name = "gaba_wrap",
	.depends_on = { "gaba" }
);


unittest()
{
	gaba_t *c = gaba_init(NULL);
	assert(c == NULL);
}

/* linear gap penalty */
unittest()
{
	gaba_t *c = gaba_init(GABA_PARAMS(GABA_SCORE_SIMPLE(1, 1, 0, 1)));
	assert(c != NULL);

	void const *lim = (void const *)0x800000000000;
	gaba_dp_t *d = gaba_dp_init(c, lim, lim);
	assert(d != NULL);

	gaba_dp_clean(d);
	gaba_clean(c);
}

unittest(with_seq_pair("GGAAAAAAAA", "AAAAAAAA"))
{
	omajinai();

	void const *lim = (void const *)0x800000000000;
	gaba_t *c = gaba_init(GABA_PARAMS(GABA_SCORE_SIMPLE(1, 1, 0, 1)));
	gaba_dp_t *d = gaba_dp_init(c, lim, lim);


	/* check fill functions and resulting scores */
	gaba_fill_t *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	assert(f->max == 6, "%lld", f->max);

	/* check traceback function is callable */
	gaba_alignment_t *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(r != NULL);

	gaba_dp_clean(d);
	gaba_clean(c);	
}

/* affine gap penalty */
unittest()
{
	gaba_t *c = gaba_init(GABA_PARAMS(GABA_SCORE_SIMPLE(1, 1, 1, 1)));
	assert(c != NULL);

	void const *lim = (void const *)0x800000000000;
	gaba_dp_t *d = gaba_dp_init(c, lim, lim);
	assert(d != NULL);

	gaba_dp_clean(d);
	gaba_clean(c);
}

unittest(with_seq_pair("GGAAAAAAAA", "AAAAAAAA"))
{
	omajinai();

	void const *lim = (void const *)0x800000000000;
	gaba_t *c = gaba_init(GABA_PARAMS(GABA_SCORE_SIMPLE(1, 1, 1, 1)));
	gaba_dp_t *d = gaba_dp_init(c, lim, lim);


	/* check fill functions and resulting scores */
	gaba_fill_t *f = gaba_dp_fill_root(d, &s->afsec, 0, &s->bfsec, 0);
	f = gaba_dp_fill(d, f, &s->afsec, &s->bftail);
	f = gaba_dp_fill(d, f, &s->aftail, &s->bftail);
	assert(f->max == 5, "%lld", f->max);

	/* check traceback function is callable */
	gaba_alignment_t *r = gaba_dp_trace(d, f, NULL, NULL);
	assert(r != NULL);

	gaba_dp_clean(d);
	gaba_clean(c);	
}

#endif	/* UNITTEST != 0 */
#endif	/* _GABA_WRAP_H_INCLUDED */

/**
 * end of gaba_wrap.c
 */
