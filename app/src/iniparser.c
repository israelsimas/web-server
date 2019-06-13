/**************************************************************************
 *
 * iniparser.c
 *
 *    Parser for ini files.
 *
 * Copyright 2013 Intelbras
 *
 **************************************************************************/

#include <ctype.h>
#include "iniparser.h"
#include <misc.h>

#define THIS_FILE "iniparser.c"

/**************************************************************************
 * DEFINITIONS
 **************************************************************************/
#define ASCIILINESZ         (1024)
#define INI_INVALID_KEY     ((char *)-1)

/**************************************************************************
 * PRIVATE
 **************************************************************************/
/**
 * Este enum armazena o status de cada linha parsed.
 */
typedef enum _line_status_ {
	LINE_UNPROCESSED,
	LINE_ERROR,
	LINE_EMPTY,
	LINE_COMMENT,
	LINE_SECTION,
	LINE_VALUE
} E_LINE_STATUS;

/**
 * @brief    Converte uma string para lowercase.
 * @param    pchConvert  String para converter.
 *
 * @return   ponteiro para uma string previamente alocada.
 */
static char *strlwc(const char *pchConvert) {
	static char pchReturn[ASCIILINESZ + 1];
	int i;

	if (pchConvert == NULL) {
		return NULL;
	}

	memset(pchReturn, 0, ASCIILINESZ + 1);
	i = 0;
	while (pchConvert[i] && i < ASCIILINESZ) {
		pchReturn[i] = (char) tolower((int) pchConvert[i]);
		i++;
	}

	pchReturn[ASCIILINESZ] = (char) 0;

	return pchReturn;
}

/**
 * @brief    Remove o branco do começo e o fim da string.
 * @param    pchParser   String para parse.
 *
 * @return   ponteiro para uma string previamente alocada.
 */
static char *strstrip(const char *pchParser) {
	static char pchReturn[ASCIILINESZ + 1];
	char *pchLast;

	if (pchParser == NULL) {
		return NULL;
	}

	while (isspace((int)*pchParser) && *pchParser) {
		pchParser++;
	}

	memset(pchReturn, 0, ASCIILINESZ + 1);
	strcpy(pchReturn, pchParser);

	pchLast = pchReturn + strlen(pchReturn);
	while (pchLast > pchReturn) {

		if (!isspace((int)*(pchLast-1)))
			break;

		pchLast--;
	}

	*pchLast = (char) 0;

	return (char *) pchReturn;
}

int iniparser_getnsec(dictionary *pDictionary) {
	int i, nsec;

	if (pDictionary == NULL) {
		return -1;
	}

	nsec = 0;
	for (i = 0; i < pDictionary->size; i++) {
		if (pDictionary->key[i] == NULL)
			continue;
		if (strchr(pDictionary->key[i], ':') == NULL) {
			nsec++;
		}
	}

	return nsec;
}

char *iniparser_getsecname(dictionary *pDictionary, int numSection) {
	int i, foundsec;

	if ((pDictionary == NULL) || (numSection < 0)) {
		return NULL;
	}

	foundsec = 0;
	for (i = 0; i < pDictionary->size; i++) {
		if (pDictionary->key[i] == NULL)
			continue;
		if (strchr(pDictionary->key[i], ':') == NULL) {
			foundsec++;
			if (foundsec > numSection)
				break;
		}
	}
	if (foundsec <= numSection) {
		return NULL;
	}

	return pDictionary->key[i];
}

void iniparser_dump(dictionary *pDictionary, FILE *pFile) {
	int i;

	if ((pDictionary == NULL) || (pFile == NULL)) {
		return;
	}

	for (i = 0; i < pDictionary->size; i++) {
		if (pDictionary->key[i] == NULL)
			continue;
		if (pDictionary->val[i] != NULL) {
			fprintf(pFile, "[%s]=[%s]\n", pDictionary->key[i], pDictionary->val[i]);
		} else {
			fprintf(pFile, "[%s]=UNDEF\n", pDictionary->key[i]);
		}
	}

	return;
}

void iniparser_dump_ini(dictionary *pDictionary, FILE *pFile) {
	int i, nsec;
	char *pchSecName;

	if ((pDictionary == NULL) || (pFile == NULL)) {
		return;
	}

	nsec = iniparser_getnsec(pDictionary);
	if (nsec < 1) {
		/* No section in file: dump all keys as they are */
		for (i = 0; i < pDictionary->size; i++) {
			if (pDictionary->key[i] == NULL)
				continue;
			fprintf(pFile, "%s = %s\n", pDictionary->key[i], pDictionary->val[i]);
		}
		return;
	}

	for (i = 0; i < nsec; i++) {
		pchSecName = iniparser_getsecname(pDictionary, i);
		iniparser_dumpsection_ini(pDictionary, pchSecName, pFile);
	}
	fprintf(pFile, "\n");

	return;
}

void iniparser_dumpsection_ini(dictionary *pDictionary, char *pchSecname, FILE *pFile) {
	char keym[ASCIILINESZ + 1];
	int j, seclen;

	if ((pDictionary == NULL) || (pFile == NULL)) {
		return;
	}

	if (!iniparser_find_entry(pDictionary, pchSecname)) {
		return;
	}

	seclen = (int) strlen(pchSecname);
	fprintf(pFile, "\n[%s]\n", pchSecname);
	sprintf(keym, "%s:", pchSecname);
	for (j = 0; j < pDictionary->size; j++) {
		if (pDictionary->key[j] == NULL)
			continue;
		if (!strncmp(pDictionary->key[j], keym, seclen + 1)) {
			fprintf(pFile, "%-30s = %s\n", pDictionary->key[j] + seclen + 1, pDictionary->val[j] ? pDictionary->val[j] : "");
		}
	}

	fprintf(pFile, "\n");

	return;
}

int iniparser_getsecnkeys(dictionary *pDictionary, char *pchSecName) {
	int j, seclen, nkeys;
	char keym[ASCIILINESZ + 1];

	nkeys = 0;

	if (pDictionary == NULL) {
		return nkeys;
	}

	if (!iniparser_find_entry(pDictionary, pchSecName)) {
		return nkeys;
	}

	seclen = (int) strlen(pchSecName);
	sprintf(keym, "%s:", pchSecName);

	for (j = 0; j < pDictionary->size; j++) {
		if (pDictionary->key[j] == NULL)
			continue;
		if (!strncmp(pDictionary->key[j], keym, seclen + 1))
			nkeys++;
	}

	return nkeys;
}

char **iniparser_getseckeys(dictionary *pDictionary, char *pchSecname) {

	char **ppKeys;
	char keym[ASCIILINESZ + 1];
	int i, j, seclen, nkeys;

	ppKeys = NULL;

	if (pDictionary == NULL) {
		return ppKeys;
	}

	if (!iniparser_find_entry(pDictionary, pchSecname)) {
		return ppKeys;
	}

	nkeys = iniparser_getsecnkeys(pDictionary, pchSecname);

	ppKeys = (char **) malloc(nkeys * sizeof(char *));

	seclen = (int) strlen(pchSecname);
	sprintf(keym, "%s:", pchSecname);

	i = 0;

	for (j = 0; j < pDictionary->size; j++) {
		if (pDictionary->key[j] == NULL)
			continue;
		if (!strncmp(pDictionary->key[j], keym, seclen + 1)) {
			ppKeys[i] = pDictionary->key[j];
			i++;
		}
	}

	return ppKeys;
}

char *iniparser_getstring(dictionary *pDictionary, const char *pchKey, char *pchDef) {
	char *pchLc_key;
	char *pchSval;

	if ((pDictionary == NULL) || (pchKey == NULL)) {
		return pchDef;
	}

	pchLc_key = strlwc(pchKey);
	pchSval = dictionary_get(pDictionary, pchLc_key, pchDef);

	return pchSval;
}

int iniparser_getint(dictionary *pDictionary, const char *pchKey, int notfound) {
	char *pchStr;

	pchStr = iniparser_getstring(pDictionary, pchKey, INI_INVALID_KEY);
	if (pchStr == INI_INVALID_KEY) {
		return notfound;
	}

	return (int) strtol(pchStr, NULL, 0);
}

double iniparser_getdouble(dictionary *pDictionary, const char *pchKey, double notfound) {
	char *pchStr;

	pchStr = iniparser_getstring(pDictionary, pchKey, INI_INVALID_KEY);
	if (pchStr == INI_INVALID_KEY) {
		return notfound;
	}

	return atof(pchStr);
}

int iniparser_getboolean(dictionary *pDictionary, const char *pchKey, int notfound) {
	char *pKey;
	int ret;

	pKey = iniparser_getstring(pDictionary, pchKey, INI_INVALID_KEY);
	if (pKey == INI_INVALID_KEY) {
		return notfound;
	}

	if ((pKey[0] == 'y') || (pKey[0] == 'Y') || (pKey[0] == '1') || (pKey[0] == 't') || (pKey[0] == 'T')) {
		ret = 1;
	} else if ((pKey[0] == 'n') || (pKey[0] == 'N') || (pKey[0] == '0') || (pKey[0] == 'f') || (pKey[0] == 'F')) {
		ret = 0;
	} else {
		ret = notfound;
	}

	return ret;
}

int iniparser_find_entry(dictionary *pIni, const char *pchEntry) {
	int found = 0;

	if (iniparser_getstring(pIni, pchEntry, INI_INVALID_KEY) != INI_INVALID_KEY) {
		found = 1;
	}

	return found;
}

int iniparser_set(dictionary *pIni, const char *pchEntry, const char *pchVal) {
	return dictionary_set(pIni, strlwc(pchEntry), pchVal);
}

void iniparser_unset(dictionary *pIni, const char *pchEntry) {
	dictionary_unset(pIni, strlwc(pchEntry));
}

/**
 * @brief    Carrega um simples linha do arquivo INI
 * @param    pchInputLine   Linha de entrada deve ser contatenável com multi-line
 * @param    pchSection     Seção
 * @param    pchKey         parâmetro
 * @param    pchValue       Valor armazenado
 *
 * @return   line_status value
 */
static E_LINE_STATUS iniparser_line(const char *pchInputLine, char *pchSection, char *pchKey, char *pchValue) {
	E_LINE_STATUS sta;
	char pchLine[ASCIILINESZ + 1];
	int len;

	strcpy(pchLine, strstrip(pchInputLine));
	len = (int) strlen(pchLine);

	sta = LINE_UNPROCESSED;
	if (len < 1) {
		/* Linha vazia */
		sta = LINE_EMPTY;
	} else if ((pchLine[0] == '#') || (pchLine[0] == ';')) {
		/* Linha comentada */
		sta = LINE_COMMENT;
	} else if ((pchLine[0] == '[') && (pchLine[len - 1] == ']')) {
		/* Nome da seção */
		sscanf(pchLine, "[%[^]]", pchSection);
		strcpy(pchSection, strstrip(pchSection));
		strcpy(pchSection, strlwc(pchSection));
		sta = LINE_SECTION;
	} else if ((sscanf(pchLine, "%[^=] = \"%[^\"]\"", pchKey, pchValue) == 2) || (sscanf(pchLine, "%[^=] = '%[^\']'", pchKey, pchValue) == 2)
			|| (sscanf(pchLine, "%[^=] = %[^;#]", pchKey, pchValue) == 2)) {
		/* Usualmente key=value, com ou sem comentários */
		strcpy(pchKey, strstrip(pchKey));
		strcpy(pchKey, strlwc(pchKey));
		strcpy(pchValue, strstrip(pchValue));

		/* sscanf não pode manipular '' ou "" como valores vazios	 */
		if (!strcmp(pchValue, "\"\"") || (!strcmp(pchValue, "''"))) {
			pchValue[0] = 0;
		}

		sta = LINE_VALUE;
	} else if ((sscanf(pchLine, "%[^=] = %[;#]", pchKey, pchValue) == 2) || (sscanf(pchLine, "%[^=] %[=]", pchKey, pchValue) == 2)) {
		/*
		 * casos especiais:
		 * key=
		 * key=;
		 * key=#
		 */
		strcpy(pchKey, strstrip(pchKey));
		strcpy(pchKey, strlwc(pchKey));
		pchValue[0] = 0;
		sta = LINE_VALUE;
	} else {
		/* Erro de sintaxe */
		sta = LINE_ERROR;
	}

	return sta;
}

dictionary * iniparser_load(const char *pchIniname) {
	FILE *pFileIn;

	char line[ASCIILINESZ + 1];
	char section[ASCIILINESZ + 1];
	char key[ASCIILINESZ + 1];
	char tmp[ASCIILINESZ + 1];
	char val[ASCIILINESZ + 1];

	int last = 0;
	int len;
	int lineno = 0;
	int errs = 0;

	dictionary *pDictionary;

	if ((pFileIn = fopen(pchIniname, "r")) == NULL) {
		LOG_ERROR("iniparser: nao pode abrir %s", pchIniname);
		return NULL;
	}

	pDictionary = dictionary_new(0);
	if (!pDictionary) {
		fclose(pFileIn);
		return NULL;
	}

	memset(line, 0, ASCIILINESZ);
	memset(section, 0, ASCIILINESZ);
	memset(key, 0, ASCIILINESZ);
	memset(val, 0, ASCIILINESZ);
	last = 0;

	while (fgets(line + last, ASCIILINESZ - last, pFileIn) != NULL) {
		lineno++;
		len = (int) strlen(line) - 1;
		if (len == 0)
			continue;

		/* Seguramente verifica buffer overflows */
		if (line[len] != '\n') {
			LOG_ERROR("iniparser: arquivo muito grande %s", pchIniname);
			dictionary_del(pDictionary);
			fclose(pFileIn);

			return NULL;
		}

		/* Recupera \n e espaços no fim da linha */
		while ((len >= 0) && ((line[len] == '\n') || (isspace(line[len])))) {
			line[len] = 0;
			len--;
		}
		/* Detcta multiplas linhas */
		if (line[len] == '\\') {
			/* Valor de multiplas linhas */
			last = len;
			continue;
		} else {
			last = 0;
		}

		switch (iniparser_line(line, section, key, val)) {
			case LINE_EMPTY:
			case LINE_COMMENT:
				break;

			case LINE_SECTION:
				errs = dictionary_set(pDictionary, section, NULL);
				break;

			case LINE_VALUE:
				sprintf(tmp, "%s:%s", section, key);
				errs = dictionary_set(pDictionary, tmp, val);
				break;

			case LINE_ERROR:
				LOG("iniparser: erro de sintaxe %s", pchIniname);
				fprintf(stderr, "-> %s\n", line);
				errs++;
				break;

			default:
				break;
		}

		memset(line, 0, ASCIILINESZ);
		last = 0;
		if (errs < 0) {
			LOG("iniparser: falha ao alocar memoria");
			break;
		}
	}

	if (errs) {
		dictionary_del(pDictionary);
		pDictionary = NULL;
	}

	fclose(pFileIn);

	return pDictionary;
}

void iniparser_freedict(dictionary *pDictionary) {
	dictionary_del(pDictionary);
}
