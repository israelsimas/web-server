/**************************************************************************
 * dictionary.h
 *
 *  Create on: 01/09/2017
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform SIP Intelbras
 *  This module implements a simple dictionary object, i.e. a list
 *  of string/string associations. This object is useful to store e.g.
 *  informations retrieved from a configuration file (ini files).
 *
 * Copyrights Intelbras, 2017
 *
 **************************************************************************/

#ifndef _DICTIONARY_H_
#define _DICTIONARY_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/

/** Maior valor para integers e doubles. */
#define MAXVALSZ    1024

/** Número mínimo alocado de entradas para um dicionário */
#define DICTMINSZ   128

/** Token inválido */
#define DICT_INVALID_KEY    ((char *)-1)

/**************************************************************************
 * TYPEDEFS
 **************************************************************************/
/**
 * @brief    dicionário de objetos
 *
 */
typedef struct _dictionary_ {
    int             n;     		/**< Número de entradas do dicionário */
    int           size;  			/**< Tamanho armazenado */
    char        **val;   			/**< Lista de valores strings */
    char        **key;   			/**< Lista de parâmetros de string */
    unsigned     *hash;  			/**< Lista de valores hash para os parâmetros */
} dictionary ;


/**************************************************************************
 * INTERNAL CALL FUNCTIONS
 **************************************************************************/

/**
 * @brief    Computa o hash key para uma string.
 * @param    pchKey   Caracteres de uma string ppara uso no parâmetro.
 *
 * @return   1 unsigned int ao menos 32 bits.
 */
unsigned dictionary_hash(const char *pchKey);

/**
 * @brief    Cria um novo objeto de dicionário.
 * @param    size    Tamanho inicial do dicionário (opcional).
 *
 * @return   1 dicionário alocado.
 */
dictionary *dictionary_new(int size);

/**
 * @brief    Remove um dicionário
 * @param    pDictionary   dicionário para ser removido.
 *
 * @return   void
 */
void dictionary_del(dictionary *pDictionary);

/**
 * @brief    Recupera o valor de um dicionário.
 * @param    pDictionary       dicionário a ser procurado.
 * @param    pchKey     Parâmetro para busca no dicionário.
 * @param    pchDef     Valor padrão para retorno se não encontrar parâmetro.
 *
 * @return   1 ponteiro de retorno.
 */
char *dictionary_get(dictionary *pDictionary, const char *pchKey, char *pchDef);

/**
 * @brief    Configura um valor de um dicionário.
 * @param    pDictionary       dicionário para modificar.
 * @param    pchKey     parâmetro a ser modificado.
 * @param    pchVal     Valor para inserir.
 *
 * @return   int     0 se Ok
 */
int dictionary_set(dictionary *pDictionary, const char *pchKey, const char *pchVal);

/**
 * @brief    Remove um parâmetro do dicionário
 * @param    pDictionary       Dicionário a ser modificado.
 * @param    pchKey     parâmetro a ser removido.
 *
 * @return   void
 */
void dictionary_unset(dictionary *pDictionary, const char *pchKey);

/**
 * @brief    Dump um dicionário para um ponteiro de arquivo.
 * @param    pDictionary   Dicionário para dump
 * @param    pFile   ponteiro do arquivo aberto.
 *
 * @return   void
 */
void dictionary_dump(dictionary *pDictionary, FILE *pFile);

#endif
