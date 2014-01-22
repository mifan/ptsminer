/*
 * Copyright (c) 2014 Girino Vey.
 *
 * All code in this file is copyrighted to me, Girino Vey, and licensed under Girino's
 * Anarchist License, available at http://girino.org/license and is available on this
 * repository as the file girino_license.txt
 *
 */

#ifdef _ECLIPSE_OPENCL_HEADER
#   include "OpenCLKernel.hpp"
#   include "opencl_cryptsha512.h"
#   include "cryptsha512_kernel_AMD.cl"
#endif

#define _OPENCL_COMPILER
#define SEARCH_SPACE_BITS (50)
#define GET_BIRTHDAY(x) (x >> (64UL - SEARCH_SPACE_BITS));
#define COLLISION_KEY_MASK 0xFF800000UL

typedef struct _collision_struct {
	uint64_t birthday;
	uint32_t nonce_a;
	uint32_t nonce_b;
} collision_struct;

kernel void kernel_clean_hash_table(global uint32_t * hash_table) {
	size_t id = get_global_id(0);
	hash_table[id] = 0;
}

// first pass, hashes
kernel void calculate_all_hashes(constant char * message,
								 global uint64_t * hashes,
								 local  uint64_t * local_hashes,
								 local sha512_ctx * local_ctx,
								 local char * tempHashes) {
	size_t id = get_global_id(0);
	size_t lid = get_local_id(0);
	uint32_t local_idx = 8* lid;
	uint32_t local_temp_idx = 36*lid;
	uint32_t nonce = (id*8);

	#pragma unroll
	for (int i = 0; i < 32; i++) tempHashes[local_temp_idx+i+4] = message[i];
	*((local uint32_t*)(tempHashes+local_temp_idx)) = nonce;

    init_ctx(local_ctx+lid);
    ctx_update(local_ctx+lid, tempHashes+local_temp_idx, 36);
    sha512_digest(local_ctx+lid, local_hashes+local_idx);

    mem_fence(CLK_LOCAL_MEM_FENCE);

    hashes[nonce] = local_hashes[local_idx];
    hashes[nonce+1] = local_hashes[local_idx+1];
    hashes[nonce+2] = local_hashes[local_idx+2];
    hashes[nonce+3] = local_hashes[local_idx+3];
    hashes[nonce+4] = local_hashes[local_idx+4];
    hashes[nonce+5] = local_hashes[local_idx+5];
    hashes[nonce+6] = local_hashes[local_idx+6];
    hashes[nonce+7] = local_hashes[local_idx+7];
}

// second pass, fill table
kernel void fill_table(global uint64_t * hashes,
						  global uint32_t * hash_table,
                          uint32_t HASH_TABLE_SIZE) {
	size_t nonce = get_global_id(0);
	unsigned long birthdayB = GET_BIRTHDAY(hashes[nonce]);
	unsigned int collisionKey = (unsigned int)((birthdayB>>18) & COLLISION_KEY_MASK);
	unsigned long birthday = birthdayB % (HASH_TABLE_SIZE);
	hash_table[birthday] = (nonce>>3) | collisionKey; // we have 6 bits available for validation
}

// third pass, lookup
kernel void find_collisions(global uint64_t * hashes,
							global uint32_t * hash_table,
							uint32_t HASH_TABLE_SIZE,
							global collision_struct * collisions,
							global uint32_t * collision_count) {
	size_t nonce = get_global_id(0);
	unsigned long birthdayB = GET_BIRTHDAY(hashes[nonce]);
	unsigned int collisionKey = (unsigned int)((birthdayB>>18) & COLLISION_KEY_MASK);
	unsigned long birthday = birthdayB % (HASH_TABLE_SIZE);
	if( hash_table[birthday] && ((hash_table[birthday]&COLLISION_KEY_MASK) == collisionKey)) {
		// collision candidate
		unsigned int nonceA = (hash_table[birthday]&~COLLISION_KEY_MASK)<<3;
		for (int i = 0; i < 8; i++) {
			unsigned long birthdayA = GET_BIRTHDAY(hashes[nonceA+i]);
			if (birthdayA == birthdayB && (nonceA+i) != nonce) {
				uint32_t pos = atomic_inc(collision_count);
				collisions[pos].nonce_b = nonce;
				collisions[pos].nonce_a = nonceA;
				collisions[pos].birthday = birthdayB;
			}
		}
	}

}