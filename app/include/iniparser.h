/**************************************************************************
 * iniparser.h
 *
 *  Create on: 01/09/2017
 *     Author: Intelbras SIP team
 *
 *  Header File to Plataform SIP Intelbras
 *
 * Copyrights Intelbras, 2017
 *
 **************************************************************************/

#ifndef _INIPARSER_H_
#define _INIPARSER_H_

/**************************************************************************
 * INCLUDES
 **************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dictionary.h"

/**************************************************************************
 * INTERNAL CALL FUNCTIONS
 **************************************************************************/

/**
 * @brief    Recuper o número de seções em um dicionário
 * @param    pDictionary   Dicionário para examinar
 *
 * @return   int Número de seções encontradas em um dicionário
 */
int iniparser_getnsec(dictionary *pDictionary);

/**
 * @brief    Recupera o nome a seção n em um dicionário.
 * @param    pDictionary   Dictionary to examine
 * @param    numSection   Section number (from 0 to nsec-1).
 *
 * @return   Pointer to char string
 */
char *iniparser_getsecname(dictionary *pDictionary, int numSection);

/**
 * @brief    Salva um dicionário para um arquivo ini
 * @param    pDictionary   Dictionary to dump
 * @param    pFile   Opened file pointer to dump to
 *
 * @return   void
 */
void iniparser_dump_ini(dictionary *pDictionary, FILE *pFile);

/**
 * @brief    Salva um dicionário para um arquivo ini
 * @param    pDictionary   Dicionário para dump
 * @param    pchSecName   Nome da seção do dicionário para dump
 * @param    pFile   Ponteiro do arquivo aberto para dump
 *
 * @return   void
 */
void iniparser_dumpsection_ini(dictionary *pDictionary, char *pchSlection, FILE *pFile);

/**
 * @brief    Dump um dicionário para ponteiro de arquivo aberto.
 * @param    pDictionary   Dicionário para dump.
 * @param    pFile   ponteiro do arquivo aberto para dump.
 *
 * @return   void
 */
void iniparser_dump(dictionary *pDictionary, FILE *pFile);

/**
 * @brief    Recupera o número de parâmetros em uma seção de um dicionário.
 * @param    pDictionary  Dicionário para examinar
 * @param    pchSecName   Nome da seção do dicionário para examinar
 *
 * @return   Número de parâmetros na seção
 */
int iniparser_getsecnkeys(dictionary *pDictionary, char *pchSection);

/**
 * @brief    Recupera o número de parâmetros em uma seção de um dicionário.
 * @param    pDictionary  Dicionário para examinar
 * @param    pchSecName   Nome da seção do dicionário para examinar
 *
 * @return   ponteiro para uma string previamente alocada
 */
char **iniparser_getseckeys(dictionary *pDictionary, char *pchSection);

/**
 * @brief    Recupera a string associada ao parâmetro
 * @param    pDictionary   Dicionário para a busca
 * @param    pchKey     Parâmetro a ser procurado
 * @param    pchDef     valor Default para retorno se o parâmetro não é encontrado.
 *
 * @return   ponteiro para uma string previamente alocada
 */
char *iniparser_getstring(dictionary *pDictionary, const char *pchKey, char *pchDef);

/**
 * @brief    Recupera a string associada para o parâmetro converte para int
 * @param    pDictionary Dicionário de busca
 * @param    pchKey parâmetro a ser procurado
 * @param    notfound Valor de retorno em caso de erro
 *
 * @return   integer
 */
int iniparser_getint(dictionary *pDictionary, const char *pchKey, int notfound);

/**
 * @brief    Recupera string associada para o parâmetro converte para double
 * @param    pDictionary Dicionário para a busca
 * @param    pchKey parâmetro procurado
 * @param    notfound Valor de retorno em caso de erro
 *
 * @return   double
 */
double iniparser_getdouble(dictionary *pDictionary, const char *pchKey, double notfound);

/**
 * @brief    Recupera string associada para o parâmetro converte para bool
 * @param    pDictionary Dicionário para a busca
 * @param    pchKey parâmetro procurado
 * @param    notfound Valor de retorno em caso de erro
 *
 * @return   double
 */
int iniparser_getboolean(dictionary *pDictionary, const char *pchKey, int notfound);

/**
 * @brief    Conjunto de entradas para um dicionário.
 * @param    pIni     Dicionário para ser modificado.
 * @param    pchEntry Entrada a ser modificada
 * @param    pchVal   Novo valor assciado.
 *
 * @return   int 0 Se Ok, -1 caso não.
 */
int iniparser_set(dictionary *pIni, const char *pchEntry, const char *pVal);

/**
 * @brief    Remove uma entrada de um dicionário
 * @param    pIni     Dicionário para ser modificado
 * @param    pchEntry Entrada a ser removida
 *
 * @return   void
 */
void iniparser_unset(dictionary *pIni, const char *pchEntry);

/**
 * @brief    Busca se uma determina entrada existe no dicionário
 * @param    pIni     Dicionário de busca
 * @param    pchEntry Nome da entrada a ser procurada
 *
 * @return   integer 1 se existe, 0 outro caso
 */
int iniparser_find_entry(dictionary *pIni, const char *pchEntry);

/**
 * @brief    Parse um arquivo ini e retorna objeto de dicionário
 * @param    pchIniname Nome do arquivo ini para ler.
 * @return   Pointer para a memória do novo dicionário
 */
dictionary *iniparser_load(const char *pIniname);

/**
 * @brief    Libera a memória para o dicionário associado
 * @param    pDictionary Dicionário a ser liberado
 *
 * @return   void
 */
void iniparser_freedict(dictionary *pDictionary);

#endif
