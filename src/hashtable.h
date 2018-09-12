/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <stdlib.h>
#include <stdint.h>

typedef uintptr_t hash_key_t;
typedef uintptr_t hash_val_t;

struct hash_entry {
    hash_key_t key;
    hash_val_t val;
};

typedef uint32_t(*hash_fn_t)(hash_key_t k);
typedef int(*eql_fn_t)(hash_key_t a, hash_key_t b);

struct hash_table {
    struct hash_entry* table;
    uint32_t* hashes;
    size_t size;
    size_t capacity;
    hash_fn_t hash_fn;
    eql_fn_t eql_fn;
};

struct hash_table* hash_table_create(hash_fn_t hash_fn, eql_fn_t eql_fn);
void hash_table_destroy(struct hash_table* ht);

void hash_table_insert(struct hash_table* ht, hash_key_t k, hash_val_t v);
hash_val_t* hash_table_search(struct hash_table* ht, hash_key_t k);
int hash_table_remove(struct hash_table* ht, hash_key_t k);
struct hash_entry* hash_table_next_entry(struct hash_table* ht, struct hash_entry* entry);

uint32_t hash_table_string_hash(hash_key_t k);
int hash_table_string_eql(hash_key_t k1, hash_key_t k2);
uint32_t hash_table_int_hash(hash_key_t k);
int hash_table_int_eql(hash_key_t k1, hash_key_t k2);

#define hash_table_foreach(ht, e) \
    for(struct hash_entry* e = hash_table_next_entry(ht, 0); e != 0; e = hash_table_next_entry(ht, e))

#endif /* ! _HASHTABLE_H_ */
