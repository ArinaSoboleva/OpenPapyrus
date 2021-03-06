/*
 * schematron.c : implementation of the Schematron schema validity checking
 *
 * See Copyright for the status of this software.
 *
 * Daniel Veillard <daniel@veillard.com>
 */

/*
 * TODO:
 * + double check the semantic, especially
 *        - multiple rules applying in a single pattern/node
 *        - the semantic of libxml2 patterns vs. XSLT production referenced
 *          by the spec.
 * + export of results in SVRL
 * + full parsing and coverage of the spec, conformance of the input to the
 *   spec
 * + divergences between the draft and the ISO proposed standard :-(
 * + hook and test include
 * + try and compare with the XSLT version
 */

#define IN_LIBXML
#include "libxml.h"
#pragma hdrstop

#ifdef LIBXML_SCHEMATRON_ENABLED
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/pattern.h>
#include <libxml/schematron.h>

#define SCHEMATRON_PARSE_OPTIONS XML_PARSE_NOENT

#define SCT_OLD_NS BAD_CAST "http://www.ascc.net/xml/schematron"

#define XML_SCHEMATRON_NS BAD_CAST "http://purl.oclc.org/dsdl/schematron"

static const xmlChar * xmlSchematronNs = XML_SCHEMATRON_NS;
static const xmlChar * xmlOldSchematronNs = SCT_OLD_NS;

#define IS_SCHEMATRON(node, elem)					\
	((node != NULL) && (node->type == XML_ELEMENT_NODE ) &&		     \
	    (node->ns != NULL) &&						\
	    (sstreq(node->name, (const xmlChar*)elem)) &&		      \
	    ((sstreq(node->ns->href, xmlSchematronNs)) ||			\
		    (sstreq(node->ns->href, xmlOldSchematronNs))))

#define NEXT_SCHEMATRON(node)						\
	while(node) {						    \
		if((node->type == XML_ELEMENT_NODE ) && (node->ns != NULL) &&	\
		    ((sstreq(node->ns->href, xmlSchematronNs)) ||		 \
			    (sstreq(node->ns->href, xmlOldSchematronNs))))		\
			break;							     \
		node = node->next;						 \
	}

/**
 * TODO:
 *
 * macro to flag unimplemented blocks
 */
#define TODO xmlGenericError(0, "Unimplemented block at %s:%d\n", __FILE__, __LINE__);

typedef enum {
	XML_SCHEMATRON_ASSERT = 1,
	XML_SCHEMATRON_REPORT = 2
} xmlSchematronTestType;

/**
 * _xmlSchematronTest:
 *
 * A Schematrons test, either an assert or a report
 */
typedef struct _xmlSchematronTest xmlSchematronTest;
typedef xmlSchematronTest * xmlSchematronTestPtr;
struct _xmlSchematronTest {
	xmlSchematronTestPtr next; /* the next test in the list */
	xmlSchematronTestType type; /* the test type */
	xmlNodePtr node;        /* the node in the tree */
	xmlChar * test;         /* the expression to test */
	xmlXPathCompExprPtr comp; /* the compiled expression */
	xmlChar * report;       /* the message to report */
};

/**
 * _xmlSchematronRule:
 *
 * A Schematrons rule
 */
typedef struct _xmlSchematronRule xmlSchematronRule;
typedef xmlSchematronRule * xmlSchematronRulePtr;
struct _xmlSchematronRule {
	xmlSchematronRulePtr next; /* the next rule in the list */
	xmlSchematronRulePtr patnext; /* the next rule in the pattern list */
	xmlNodePtr node;        /* the node in the tree */
	xmlChar * context;      /* the context evaluation rule */
	xmlSchematronTestPtr tests; /* the list of tests */
	xmlPatternPtr pattern;  /* the compiled pattern associated */
	xmlChar * report;       /* the message to report */
};

/**
 * _xmlSchematronPattern:
 *
 * A Schematrons pattern
 */
typedef struct _xmlSchematronPattern xmlSchematronPattern;
typedef xmlSchematronPattern * xmlSchematronPatternPtr;
struct _xmlSchematronPattern {
	xmlSchematronPatternPtr next; /* the next pattern in the list */
	xmlSchematronRulePtr rules; /* the list of rules */
	xmlChar * name;         /* the name of the pattern */
};

/**
 * _xmlSchematron:
 *
 * A Schematrons definition
 */
struct _xmlSchematron {
	const xmlChar * name;   /* schema name */
	int preserve;           /* was the document passed by the user */
	xmlDocPtr doc;          /* pointer to the parsed document */
	int flags;              /* specific to this schematron */
	void * _private;        /* unused by the library */
	xmlDictPtr dict;        /* the dictionnary used internally */
	const xmlChar * title;  /* the title if any */
	int nbNs;               /* the number of namespaces */
	int nbPattern;          /* the number of patterns */
	xmlSchematronPatternPtr patterns; /* the patterns found */
	xmlSchematronRulePtr rules; /* the rules gathered */
	int nbNamespaces;       /* number of namespaces in the array */
	int maxNamespaces;      /* size of the array */
	const xmlChar ** namespaces; /* the array of namespaces */
};

/**
 * xmlSchematronValidCtxt:
 *
 * A Schematrons validation context
 */
struct _xmlSchematronValidCtxt {
	int type;
	int flags;              /* an or of xmlSchematronValidOptions */
	xmlDictPtr dict;
	int nberrors;
	int err;
	xmlSchematronPtr schema;
	xmlXPathContextPtr xctxt;
	FILE * outputFile;      /* if using XML_SCHEMATRON_OUT_FILE */
	xmlBufferPtr outputBuffer; /* if using XML_SCHEMATRON_OUT_BUFFER */
#ifdef LIBXML_OUTPUT_ENABLED
	xmlOutputWriteCallback iowrite; /* if using XML_SCHEMATRON_OUT_IO */
	xmlOutputCloseCallback ioclose;
#endif
	void * ioctx;
	/* error reporting data */
	void * userData;                 /* user specific data block */
	xmlSchematronValidityErrorFunc error; /* the callback in case of errors */
	xmlSchematronValidityWarningFunc warning; /* callback in case of warning */
	xmlStructuredErrorFunc serror;   /* the structured function */
};

struct _xmlSchematronParserCtxt {
	int type;
	const xmlChar * URL;
	xmlDocPtr doc;
	int preserve;           /* Whether the doc should be freed  */
	const char * buffer;
	int size;
	xmlDictPtr dict;        /* dictionnary for interned string names */
	int nberrors;
	int err;
	xmlXPathContextPtr xctxt; /* the XPath context used for compilation */
	xmlSchematronPtr schema;
	int nbNamespaces;       /* number of namespaces in the array */
	int maxNamespaces;      /* size of the array */
	const xmlChar ** namespaces; /* the array of namespaces */
	int nbIncludes;         /* number of includes in the array */
	int maxIncludes;        /* size of the array */
	xmlNodePtr * includes;  /* the array of includes */
	/* error reporting data */
	void * userData;                 /* user specific data block */
	xmlSchematronValidityErrorFunc error; /* the callback in case of errors */
	xmlSchematronValidityWarningFunc warning; /* callback in case of warning */
	xmlStructuredErrorFunc serror;   /* the structured function */
};

#define XML_STRON_CTXT_PARSER 1
#define XML_STRON_CTXT_VALIDATOR 2

/************************************************************************
*									*
*			Error reporting					*
*									*
************************************************************************/

/**
 * xmlSchematronPErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void xmlSchematronPErrMemory(xmlSchematronParserCtxtPtr ctxt, const char * extra, xmlNodePtr node)
{
	if(ctxt)
		ctxt->nberrors++;
	__xmlSimpleError(XML_FROM_SCHEMASP, XML_ERR_NO_MEMORY, node, NULL, extra);
}

/**
 * xmlSchematronPErr:
 * @ctxt: the parsing context
 * @node: the context node
 * @error: the error code
 * @msg: the error message
 * @str1: extra data
 * @str2: extra data
 *
 * Handle a parser error
 */
static void xmlSchematronPErr(xmlSchematronParserCtxtPtr ctxt, xmlNodePtr node, int error,
    const char * msg, const xmlChar * str1, const xmlChar * str2)
{
	xmlGenericErrorFunc channel = NULL;
	xmlStructuredErrorFunc schannel = NULL;
	void * data = NULL;
	if(ctxt) {
		ctxt->nberrors++;
		channel = ctxt->error;
		data = ctxt->userData;
		schannel = ctxt->serror;
	}
	__xmlRaiseError(schannel, channel, data, ctxt, node, XML_FROM_SCHEMASP, error, XML_ERR_ERROR, NULL, 0, 
		(const char*)str1, (const char*)str2, NULL, 0, 0, msg, str1, str2);
}

/**
 * xmlSchematronVTypeErrMemory:
 * @node: a context node
 * @extra:  extra informations
 *
 * Handle an out of memory condition
 */
static void xmlSchematronVErrMemory(xmlSchematronValidCtxtPtr ctxt, const char * extra, xmlNodePtr node)
{
	if(ctxt) {
		ctxt->nberrors++;
		ctxt->err = XML_SCHEMAV_INTERNAL;
	}
	__xmlSimpleError(XML_FROM_SCHEMASV, XML_ERR_NO_MEMORY, node, NULL,
	    extra);
}

/************************************************************************
*									*
*		Parsing and compilation of the Schematrontrons		*
*									*
************************************************************************/

/**
 * xmlSchematronAddTest:
 * @ctxt: the schema parsing context
 * @type:  the type of test
 * @rule:  the parent rule
 * @node:  the node hosting the test
 * @test: the associated test
 * @report: the associated report string
 *
 * Add a test to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronTestPtr xmlSchematronAddTest(xmlSchematronParserCtxtPtr ctxt,
    xmlSchematronTestType type,
    xmlSchematronRulePtr rule,
    xmlNodePtr node, xmlChar * test, xmlChar * report)
{
	xmlSchematronTestPtr ret;
	xmlXPathCompExprPtr comp;

	if(!ctxt || (rule == NULL) || (node == NULL) ||
	    (test == NULL))
		return 0;

	/*
	 * try first to compile the test expression
	 */
	comp = xmlXPathCtxtCompile(ctxt->xctxt, test);
	if(comp == NULL) {
		xmlSchematronPErr(ctxt, node,
		    XML_SCHEMAP_NOROOT,
		    "Failed to compile test expression %s",
		    test, NULL);
		return 0;
	}

	ret = (xmlSchematronTestPtr)SAlloc::M(sizeof(xmlSchematronTest));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema test", node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronTest));
	ret->type = type;
	ret->node = node;
	ret->test = test;
	ret->comp = comp;
	ret->report = report;
	ret->next = NULL;
	if(rule->tests == NULL) {
		rule->tests = ret;
	}
	else {
		xmlSchematronTestPtr prev = rule->tests;

		while(prev->next != NULL)
			prev = prev->next;
		prev->next = ret;
	}
	return ret;
}

/**
 * xmlSchematronFreeTests:
 * @tests:  a list of tests
 *
 * Free a list of tests.
 */
static void xmlSchematronFreeTests(xmlSchematronTestPtr tests)
{
	xmlSchematronTestPtr next;
	while(tests != NULL) {
		next = tests->next;
		SAlloc::F(tests->test);
		xmlXPathFreeCompExpr(tests->comp);
		SAlloc::F(tests->report);
		SAlloc::F(tests);
		tests = next;
	}
}

/**
 * xmlSchematronAddRule:
 * @ctxt: the schema parsing context
 * @schema:  a schema structure
 * @node:  the node hosting the rule
 * @context: the associated context string
 * @report: the associated report string
 *
 * Add a rule to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronRulePtr xmlSchematronAddRule(xmlSchematronParserCtxtPtr ctxt, xmlSchematronPtr schema,
    xmlSchematronPatternPtr pat, xmlNodePtr node,
    xmlChar * context, xmlChar * report)
{
	xmlSchematronRulePtr ret;
	xmlPatternPtr pattern;
	if(!ctxt || (schema == NULL) || (node == NULL) || (context == NULL))
		return 0;
	/*
	 * Try first to compile the pattern
	 */
	pattern = xmlPatterncompile(context, ctxt->dict, XML_PATTERN_XPATH, ctxt->namespaces);
	if(pattern == NULL) {
		xmlSchematronPErr(ctxt, node, XML_SCHEMAP_NOROOT, "Failed to compile context expression %s", context, NULL);
	}
	ret = (xmlSchematronRulePtr)SAlloc::M(sizeof(xmlSchematronRule));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema rule", node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronRule));
	ret->node = node;
	ret->context = context;
	ret->pattern = pattern;
	ret->report = report;
	ret->next = NULL;
	if(schema->rules == NULL) {
		schema->rules = ret;
	}
	else {
		xmlSchematronRulePtr prev = schema->rules;
		while(prev->next != NULL)
			prev = prev->next;
		prev->next = ret;
	}
	ret->patnext = NULL;
	if(pat->rules == NULL) {
		pat->rules = ret;
	}
	else {
		xmlSchematronRulePtr prev = pat->rules;
		while(prev->patnext != NULL)
			prev = prev->patnext;
		prev->patnext = ret;
	}
	return ret;
}

/**
 * xmlSchematronFreeRules:
 * @rules:  a list of rules
 *
 * Free a list of rules.
 */
static void xmlSchematronFreeRules(xmlSchematronRulePtr rules)
{
	xmlSchematronRulePtr next;
	while(rules != NULL) {
		next = rules->next;
		if(rules->tests)
			xmlSchematronFreeTests(rules->tests);
		SAlloc::F(rules->context);
		xmlFreePattern(rules->pattern);
		SAlloc::F(rules->report);
		SAlloc::F(rules);
		rules = next;
	}
}

/**
 * xmlSchematronAddPattern:
 * @ctxt: the schema parsing context
 * @schema:  a schema structure
 * @node:  the node hosting the pattern
 * @id: the id or name of the pattern
 *
 * Add a pattern to a schematron
 *
 * Returns the new pointer or NULL in case of error
 */
static xmlSchematronPatternPtr xmlSchematronAddPattern(xmlSchematronParserCtxtPtr ctxt,
    xmlSchematronPtr schema, xmlNodePtr node, xmlChar * name)
{
	xmlSchematronPatternPtr ret;
	if(!ctxt || (schema == NULL) || (node == NULL) || (name == NULL))
		return 0;
	ret = (xmlSchematronPatternPtr)SAlloc::M(sizeof(xmlSchematronPattern));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema pattern", node);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronPattern));
	ret->name = name;
	ret->next = NULL;
	if(schema->patterns == NULL) {
		schema->patterns = ret;
	}
	else {
		xmlSchematronPatternPtr prev = schema->patterns;
		while(prev->next != NULL)
			prev = prev->next;
		prev->next = ret;
	}
	return ret;
}

/**
 * xmlSchematronFreePatterns:
 * @patterns:  a list of patterns
 *
 * Free a list of patterns.
 */
static void xmlSchematronFreePatterns(xmlSchematronPatternPtr patterns) {
	xmlSchematronPatternPtr next;

	while(patterns != NULL) {
		next = patterns->next;
		if(patterns->name != NULL)
			SAlloc::F(patterns->name);
		SAlloc::F(patterns);
		patterns = next;
	}
}

/**
 * xmlSchematronNewSchematron:
 * @ctxt:  a schema validation context
 *
 * Allocate a new Schematron structure.
 *
 * Returns the newly allocated structure or NULL in case or error
 */
static xmlSchematronPtr xmlSchematronNewSchematron(xmlSchematronParserCtxtPtr ctxt)
{
	xmlSchematronPtr ret = (xmlSchematronPtr)SAlloc::M(sizeof(xmlSchematron));
	if(!ret) {
		xmlSchematronPErrMemory(ctxt, "allocating schema", NULL);
	}
	else {
		memzero(ret, sizeof(xmlSchematron));
		ret->dict = ctxt->dict;
		xmlDictReference(ret->dict);
	}
	return ret;
}

/**
 * xmlSchematronFree:
 * @schema:  a schema structure
 *
 * Deallocate a Schematron structure.
 */
void xmlSchematronFree(xmlSchematronPtr schema)
{
	if(schema) {
		if(schema->doc && !(schema->preserve))
			xmlFreeDoc(schema->doc);
		SAlloc::F((char**)schema->namespaces);
		xmlSchematronFreeRules(schema->rules);
		xmlSchematronFreePatterns(schema->patterns);
		xmlDictFree(schema->dict);
		SAlloc::F(schema);
	}
}

/**
 * xmlSchematronNewParserCtxt:
 * @URL:  the location of the schema
 *
 * Create an XML Schematrons parse context for that file/resource expected
 * to contain an XML Schematrons file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewParserCtxt(const char * URL)
{
	xmlSchematronParserCtxtPtr ret;
	if(URL == NULL)
		return 0;
	ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
	if(!ret) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser context", NULL);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronParserCtxt));
	ret->type = XML_STRON_CTXT_PARSER;
	ret->dict = xmlDictCreate();
	ret->URL = xmlDictLookup(ret->dict, (const xmlChar*)URL, -1);
	ret->includes = NULL;
	ret->xctxt = xmlXPathNewContext(NULL);
	if(ret->xctxt == NULL) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", NULL);
		xmlSchematronFreeParserCtxt(ret);
		return 0;
	}
	ret->xctxt->flags = XML_XPATH_CHECKNS;
	return ret;
}

/**
 * xmlSchematronNewMemParserCtxt:
 * @buffer:  a pointer to a char array containing the schemas
 * @size:  the size of the array
 *
 * Create an XML Schematrons parse context for that memory buffer expected
 * to contain an XML Schematrons file.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewMemParserCtxt(const char * buffer, int size)
{
	xmlSchematronParserCtxtPtr ret = 0;
	if(buffer && size > 0) {
		ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
		if(!ret) {
			xmlSchematronPErrMemory(NULL, "allocating schema parser context", NULL);
		}
		else {
			memzero(ret, sizeof(xmlSchematronParserCtxt));
			ret->buffer = buffer;
			ret->size = size;
			ret->dict = xmlDictCreate();
			ret->xctxt = xmlXPathNewContext(NULL);
			if(ret->xctxt == NULL) {
				xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", NULL);
				xmlSchematronFreeParserCtxt(ret);
				ret = 0;
			}
		}
	}
	return ret;
}

/**
 * xmlSchematronNewDocParserCtxt:
 * @doc:  a preparsed document tree
 *
 * Create an XML Schematrons parse context for that document.
 * NB. The document may be modified during the parsing process.
 *
 * Returns the parser context or NULL in case of error
 */
xmlSchematronParserCtxtPtr xmlSchematronNewDocParserCtxt(xmlDocPtr doc)
{
	xmlSchematronParserCtxtPtr ret = 0;
	if(doc) {
		ret = (xmlSchematronParserCtxtPtr)SAlloc::M(sizeof(xmlSchematronParserCtxt));
		if(!ret) {
			xmlSchematronPErrMemory(NULL, "allocating schema parser context", NULL);
		}
		else {
			memzero(ret, sizeof(xmlSchematronParserCtxt));
			ret->doc = doc;
			ret->dict = xmlDictCreate();
			/* The application has responsibility for the document */
			ret->preserve = 1;
			ret->xctxt = xmlXPathNewContext(doc);
			if(ret->xctxt == NULL) {
				xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context", NULL);
				xmlSchematronFreeParserCtxt(ret);
				ret = 0;
			}
		}
	}
	return ret;
}

/**
 * xmlSchematronFreeParserCtxt:
 * @ctxt:  the schema parser context
 *
 * Free the resources associated to the schema parser context
 */
void xmlSchematronFreeParserCtxt(xmlSchematronParserCtxtPtr ctxt)
{
	if(ctxt) {
		if(ctxt->doc && !ctxt->preserve)
			xmlFreeDoc(ctxt->doc);
		if(ctxt->xctxt) {
			xmlXPathFreeContext(ctxt->xctxt);
		}
		SAlloc::F(ctxt->namespaces);
		xmlDictFree(ctxt->dict);
		SAlloc::F(ctxt);
	}
}

#if 0
/**
 * xmlSchematronPushInclude:
 * @ctxt:  the schema parser context
 * @doc:  the included document
 * @cur:  the current include node
 *
 * Add an included document
 */
static void xmlSchematronPushInclude(xmlSchematronParserCtxtPtr ctxt, xmlDocPtr doc, xmlNodePtr cur)
{
	if(ctxt->includes == NULL) {
		ctxt->maxIncludes = 10;
		ctxt->includes = (xmlNodePtr*)SAlloc::M(ctxt->maxIncludes * 2 * sizeof(xmlNode *));
		if(ctxt->includes == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser includes", NULL);
			return;
		}
		ctxt->nbIncludes = 0;
	}
	else if(ctxt->nbIncludes + 2 >= ctxt->maxIncludes) {
		xmlNodePtr * tmp = (xmlNodePtr*)SAlloc::R(ctxt->includes, ctxt->maxIncludes * 4 * sizeof(xmlNode *));
		if(tmp == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser includes", NULL);
			return;
		}
		ctxt->includes = tmp;
		ctxt->maxIncludes *= 2;
	}
	ctxt->includes[2 * ctxt->nbIncludes] = cur;
	ctxt->includes[2 * ctxt->nbIncludes + 1] = (xmlNode *)doc;
	ctxt->nbIncludes++;
}

/**
 * xmlSchematronPopInclude:
 * @ctxt:  the schema parser context
 *
 * Pop an include level. The included document is being freed
 *
 * Returns the node immediately following the include or NULL if the
 *         include list was empty.
 */
static xmlNodePtr xmlSchematronPopInclude(xmlSchematronParserCtxtPtr ctxt)
{
	xmlDocPtr doc;
	xmlNodePtr ret;
	if(ctxt->nbIncludes <= 0)
		return 0;
	ctxt->nbIncludes--;
	doc = (xmlDocPtr)ctxt->includes[2 * ctxt->nbIncludes + 1];
	ret = ctxt->includes[2 * ctxt->nbIncludes];
	xmlFreeDoc(doc);
	if(ret)
		ret = ret->next;
	if(!ret)
		return(xmlSchematronPopInclude(ctxt));
	return ret;
}

#endif

/**
 * xmlSchematronAddNamespace:
 * @ctxt:  the schema parser context
 * @prefix:  the namespace prefix
 * @ns:  the namespace name
 *
 * Add a namespace definition in the context
 */
static void xmlSchematronAddNamespace(xmlSchematronParserCtxtPtr ctxt, const xmlChar * prefix, const xmlChar * ns)
{
	if(ctxt->namespaces == NULL) {
		ctxt->maxNamespaces = 10;
		ctxt->namespaces = (const xmlChar**)SAlloc::M(ctxt->maxNamespaces * 2 * sizeof(const xmlChar *));
		if(ctxt->namespaces == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser namespaces", NULL);
			return;
		}
		ctxt->nbNamespaces = 0;
	}
	else if(ctxt->nbNamespaces + 2 >= ctxt->maxNamespaces) {
		const xmlChar ** tmp = (const xmlChar**)SAlloc::R((xmlChar**)ctxt->namespaces, ctxt->maxNamespaces * 4 * sizeof(const xmlChar *));
		if(tmp == NULL) {
			xmlSchematronPErrMemory(NULL, "allocating parser namespaces", NULL);
			return;
		}
		ctxt->namespaces = tmp;
		ctxt->maxNamespaces *= 2;
	}
	ctxt->namespaces[2 * ctxt->nbNamespaces] = xmlDictLookup(ctxt->dict, ns, -1);
	ctxt->namespaces[2 * ctxt->nbNamespaces + 1] = xmlDictLookup(ctxt->dict, prefix, -1);
	ctxt->nbNamespaces++;
	ctxt->namespaces[2 * ctxt->nbNamespaces] = NULL;
	ctxt->namespaces[2 * ctxt->nbNamespaces + 1] = NULL;
}

/**
 * xmlSchematronParseRule:
 * @ctxt:  a schema validation context
 * @rule:  the rule node
 *
 * parse a rule element
 */
static void xmlSchematronParseRule(xmlSchematronParserCtxtPtr ctxt, xmlSchematronPatternPtr pattern, xmlNodePtr rule)
{
	xmlNodePtr cur;
	int nbChecks = 0;
	xmlChar * test;
	xmlChar * context;
	xmlChar * report;
	xmlSchematronRulePtr ruleptr;
	xmlSchematronTestPtr testptr;
	if(!ctxt || (rule == NULL))
		return;
	context = xmlGetNoNsProp(rule, BAD_CAST "context");
	if(context == NULL) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has no context attribute", NULL, NULL);
		return;
	}
	else if(context[0] == 0) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has an empty context attribute", NULL, NULL);
		SAlloc::F(context);
		return;
	}
	else {
		ruleptr = xmlSchematronAddRule(ctxt, ctxt->schema, pattern, rule, context, NULL);
		if(ruleptr == NULL) {
			SAlloc::F(context);
			return;
		}
	}
	cur = rule->children;
	NEXT_SCHEMATRON(cur);
	while(cur) {
		if(IS_SCHEMATRON(cur, "assert")) {
			nbChecks++;
			test = xmlGetNoNsProp(cur, BAD_CAST "test");
			if(test == NULL) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has no test attribute", NULL, NULL);
			}
			else if(test[0] == 0) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has an empty test attribute", NULL, NULL);
				SAlloc::F(test);
			}
			else {
				/* TODO will need dynamic processing instead */
				report = xmlNodeGetContent(cur);
				testptr = xmlSchematronAddTest(ctxt, XML_SCHEMATRON_ASSERT, ruleptr, cur, test, report);
				if(testptr == NULL)
					SAlloc::F(test);
			}
		}
		else if(IS_SCHEMATRON(cur, "report")) {
			nbChecks++;
			test = xmlGetNoNsProp(cur, BAD_CAST "test");
			if(test == NULL) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has no test attribute", NULL, NULL);
			}
			else if(test[0] == 0) {
				xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "assert has an empty test attribute", NULL, NULL);
				SAlloc::F(test);
			}
			else {
				/* TODO will need dynamic processing instead */
				report = xmlNodeGetContent(cur);
				testptr = xmlSchematronAddTest(ctxt, XML_SCHEMATRON_REPORT, ruleptr, cur, test, report);
				if(testptr == NULL)
					SAlloc::F(test);
			}
		}
		else {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Expecting an assert or a report element instead of %s", cur->name, NULL);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(nbChecks == 0) {
		xmlSchematronPErr(ctxt, rule, XML_SCHEMAP_NOROOT, "rule has no assert nor report element", NULL, NULL);
	}
}

/**
 * xmlSchematronParsePattern:
 * @ctxt:  a schema validation context
 * @pat:  the pattern node
 *
 * parse a pattern element
 */
static void xmlSchematronParsePattern(xmlSchematronParserCtxtPtr ctxt, xmlNodePtr pat)
{
	xmlNodePtr cur;
	xmlSchematronPatternPtr pattern;
	int nbRules = 0;
	xmlChar * id;
	if(!ctxt || (pat == NULL))
		return;
	id = xmlGetNoNsProp(pat, BAD_CAST "id");
	if(id == NULL) {
		id = xmlGetNoNsProp(pat, BAD_CAST "name");
	}
	pattern = xmlSchematronAddPattern(ctxt, ctxt->schema, pat, id);
	if(pattern == NULL) {
		SAlloc::F(id);
		return;
	}
	cur = pat->children;
	NEXT_SCHEMATRON(cur);
	while(cur) {
		if(IS_SCHEMATRON(cur, "rule")) {
			xmlSchematronParseRule(ctxt, pattern, cur);
			nbRules++;
		}
		else {
			xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Expecting a rule element instead of %s", cur->name, NULL);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(nbRules == 0) {
		xmlSchematronPErr(ctxt, pat, XML_SCHEMAP_NOROOT, "Pattern has no rule element", NULL, NULL);
	}
}

#if 0
/**
 * xmlSchematronLoadInclude:
 * @ctxt:  a schema validation context
 * @cur:  the include element
 *
 * Load the include document, Push the current pointer
 *
 * Returns the updated node pointer
 */
static xmlNodePtr xmlSchematronLoadInclude(xmlSchematronParserCtxtPtr ctxt, xmlNodePtr cur)
{
	xmlNodePtr ret = NULL;
	xmlDocPtr doc = NULL;
	xmlChar * href = NULL;
	xmlChar * base = NULL;
	xmlChar * URI = NULL;
	if(!ctxt || (cur == NULL))
		return 0;
	href = xmlGetNoNsProp(cur, BAD_CAST "href");
	if(href == NULL) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_NOROOT, "Include has no href attribute", NULL, NULL);
		return(cur->next);
	}

	/* do the URI base composition, load and find the root */
	base = xmlNodeGetBase(cur->doc, cur);
	URI = xmlBuildURI(href, base);
	doc = xmlReadFile((const char*)URI, NULL, SCHEMATRON_PARSE_OPTIONS);
	if(doc == NULL) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_FAILED_LOAD, "could not load include '%s'.\n", URI, NULL);
		goto done;
	}
	ret = xmlDocGetRootElement(doc);
	if(!ret) {
		xmlSchematronPErr(ctxt, cur, XML_SCHEMAP_FAILED_LOAD, "could not find root from include '%s'.\n", URI, NULL);
		goto done;
	}

	/* Success, push the include for rollback on exit */
	xmlSchematronPushInclude(ctxt, doc, cur);

done:
	if(!ret) {
		xmlFreeDoc(doc);
	}
	SAlloc::F(href);
	SAlloc::F(base);
	SAlloc::F(URI);
	return ret;
}

#endif

/**
 * xmlSchematronParse:
 * @ctxt:  a schema validation context
 *
 * parse a schema definition resource and build an internal
 * XML Shema struture which can be used to validate instances.
 *
 * Returns the internal XML Schematron structure built from the resource or
 *         NULL in case of error
 */
xmlSchematronPtr xmlSchematronParse(xmlSchematronParserCtxtPtr ctxt)
{
	xmlSchematronPtr ret = NULL;
	xmlDocPtr doc;
	xmlNodePtr root, cur;
	int preserve = 0;

	if(!ctxt)
		return 0;

	ctxt->nberrors = 0;

	/*
	 * First step is to parse the input document into an DOM/Infoset
	 */
	if(ctxt->URL != NULL) {
		doc = xmlReadFile((const char*)ctxt->URL, NULL,
		    SCHEMATRON_PARSE_OPTIONS);
		if(doc == NULL) {
			xmlSchematronPErr(ctxt, NULL,
			    XML_SCHEMAP_FAILED_LOAD,
			    "xmlSchematronParse: could not load '%s'.\n",
			    ctxt->URL, NULL);
			return 0;
		}
		ctxt->preserve = 0;
	}
	else if(ctxt->buffer != NULL) {
		doc = xmlReadMemory(ctxt->buffer, ctxt->size, NULL, NULL,
		    SCHEMATRON_PARSE_OPTIONS);
		if(doc == NULL) {
			xmlSchematronPErr(ctxt, NULL,
			    XML_SCHEMAP_FAILED_PARSE,
			    "xmlSchematronParse: could not parse.\n",
			    NULL, NULL);
			return 0;
		}
		doc->URL = xmlStrdup(BAD_CAST "in_memory_buffer");
		ctxt->URL = xmlDictLookup(ctxt->dict, BAD_CAST "in_memory_buffer", -1);
		ctxt->preserve = 0;
	}
	else if(ctxt->doc != NULL) {
		doc = ctxt->doc;
		preserve = 1;
		ctxt->preserve = 1;
	}
	else {
		xmlSchematronPErr(ctxt, NULL,
		    XML_SCHEMAP_NOTHING_TO_PARSE,
		    "xmlSchematronParse: could not parse.\n",
		    NULL, NULL);
		return 0;
	}

	/*
	 * Then extract the root and Schematron parse it
	 */
	root = xmlDocGetRootElement(doc);
	if(root == NULL) {
		xmlSchematronPErr(ctxt, (xmlNode *)doc,
		    XML_SCHEMAP_NOROOT,
		    "The schema has no document element.\n", NULL, NULL);
		if(!preserve) {
			xmlFreeDoc(doc);
		}
		return 0;
	}

	if(!IS_SCHEMATRON(root, "schema")) {
		xmlSchematronPErr(ctxt, root,
		    XML_SCHEMAP_NOROOT,
		    "The XML document '%s' is not a XML schematron document",
		    ctxt->URL, NULL);
		goto exit;
	}
	ret = xmlSchematronNewSchematron(ctxt);
	if(!ret)
		goto exit;
	ctxt->schema = ret;

	/*
	 * scan the schema elements
	 */
	cur = root->children;
	NEXT_SCHEMATRON(cur);
	if(IS_SCHEMATRON(cur, "title")) {
		xmlChar * title = xmlNodeGetContent(cur);
		if(title != NULL) {
			ret->title = xmlDictLookup(ret->dict, title, -1);
			SAlloc::F(title);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	while(IS_SCHEMATRON(cur, "ns")) {
		xmlChar * prefix = xmlGetNoNsProp(cur, BAD_CAST "prefix");
		xmlChar * uri = xmlGetNoNsProp(cur, BAD_CAST "uri");
		if((uri == NULL) || (uri[0] == 0)) {
			xmlSchematronPErr(ctxt, cur,
			    XML_SCHEMAP_NOROOT,
			    "ns element has no uri", NULL, NULL);
		}
		if((prefix == NULL) || (prefix[0] == 0)) {
			xmlSchematronPErr(ctxt, cur,
			    XML_SCHEMAP_NOROOT,
			    "ns element has no prefix", NULL, NULL);
		}
		if((prefix) && (uri)) {
			xmlXPathRegisterNs(ctxt->xctxt, prefix, uri);
			xmlSchematronAddNamespace(ctxt, prefix, uri);
			ret->nbNs++;
		}
		if(uri)
			SAlloc::F(uri);
		if(prefix)
			SAlloc::F(prefix);
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	while(cur) {
		if(IS_SCHEMATRON(cur, "pattern")) {
			xmlSchematronParsePattern(ctxt, cur);
			ret->nbPattern++;
		}
		else {
			xmlSchematronPErr(ctxt, cur,
			    XML_SCHEMAP_NOROOT,
			    "Expecting a pattern element instead of %s", cur->name, NULL);
		}
		cur = cur->next;
		NEXT_SCHEMATRON(cur);
	}
	if(ret->nbPattern == 0) {
		xmlSchematronPErr(ctxt, root,
		    XML_SCHEMAP_NOROOT,
		    "The schematron document '%s' has no pattern",
		    ctxt->URL, NULL);
		goto exit;
	}
	/* the original document must be kept for reporting */
	ret->doc = doc;
	if(preserve) {
		ret->preserve = 1;
	}
	preserve = 1;

exit:
	if(!preserve) {
		xmlFreeDoc(doc);
	}
	if(ret) {
		if(ctxt->nberrors != 0) {
			xmlSchematronFree(ret);
			ret = NULL;
		}
		else {
			ret->namespaces = ctxt->namespaces;
			ret->nbNamespaces = ctxt->nbNamespaces;
			ctxt->namespaces = NULL;
		}
	}
	return ret;
}

/************************************************************************
*									*
*		Schematrontron Reports handler				*
*									*
************************************************************************/

static xmlNodePtr xmlSchematronGetNode(xmlSchematronValidCtxtPtr ctxt, xmlNodePtr cur, const xmlChar * xpath)
{
	xmlNodePtr node = NULL;
	if(ctxt && cur && xpath) {
		ctxt->xctxt->doc = cur->doc;
		ctxt->xctxt->node = cur;
		xmlXPathObjectPtr ret = xmlXPathEval(xpath, ctxt->xctxt);
		if(ret) {
			if((ret->type == XPATH_NODESET) && ret->nodesetval && (ret->nodesetval->nodeNr > 0))
				node = ret->nodesetval->nodeTab[0];
			xmlXPathFreeObject(ret);
		}
	}
	return node;
}

/**
 * xmlSchematronReportOutput:
 * @ctxt: the validation context
 * @cur: the current node tested
 * @msg: the message output
 *
 * Output part of the report to whatever channel the user selected
 */
static void xmlSchematronReportOutput(xmlSchematronValidCtxtPtr ctxt ATTRIBUTE_UNUSED,
    xmlNodePtr cur ATTRIBUTE_UNUSED,
    const char * msg) {
	/* TODO */
	fprintf(stderr, "%s", msg);
}

/**
 * xmlSchematronFormatReport:
 * @ctxt:  the validation context
 * @test: the test node
 * @cur: the current node tested
 *
 * Build the string being reported to the user.
 *
 * Returns a report string or NULL in case of error. The string needs
 *         to be deallocated by teh caller
 */
static xmlChar * xmlSchematronFormatReport(xmlSchematronValidCtxtPtr ctxt, xmlNodePtr test, xmlNodePtr cur)
{
	xmlChar * ret = NULL;
	xmlNodePtr child, node;
	if((test == NULL) || (cur == NULL))
		return ret;
	child = test->children;
	while(child != NULL) {
		if((child->type == XML_TEXT_NODE) || (child->type == XML_CDATA_SECTION_NODE))
			ret = xmlStrcat(ret, child->content);
		else if(IS_SCHEMATRON(child, "name")) {
			xmlChar * path = xmlGetNoNsProp(child, BAD_CAST "path");
			node = cur;
			if(path != NULL) {
				node = xmlSchematronGetNode(ctxt, cur, path);
				SETIFZ(node, cur);
				SAlloc::F(path);
			}
			if((node->ns == NULL) || (node->ns->prefix == NULL))
				ret = xmlStrcat(ret, node->name);
			else {
				ret = xmlStrcat(ret, node->ns->prefix);
				ret = xmlStrcat(ret, BAD_CAST ":");
				ret = xmlStrcat(ret, node->name);
			}
		}
		else {
			child = child->next;
			continue;
		}

		/*
		 * remove superfluous \n
		 */
		if(ret) {
			int len = sstrlen(ret);
			xmlChar c;
			if(len > 0) {
				c = ret[len - 1];
				if((c == ' ') || (c == '\n') || (c == '\r') || (c == '\t')) {
					while((c == ' ') || (c == '\n') ||
					    (c == '\r') || (c == '\t')) {
						len--;
						if(len == 0)
							break;
						c = ret[len - 1];
					}
					ret[len] = ' ';
					ret[len + 1] = 0;
				}
			}
		}

		child = child->next;
	}
	return ret;
}

/**
 * xmlSchematronReportSuccess:
 * @ctxt:  the validation context
 * @test: the compiled test
 * @cur: the current node tested
 * @success: boolean value for the result
 *
 * called from the validation engine when an assert or report test have
 * been done.
 */
static void xmlSchematronReportSuccess(xmlSchematronValidCtxtPtr ctxt,
    xmlSchematronTestPtr test, xmlNodePtr cur, xmlSchematronPatternPtr pattern, int success)
{
	if(!ctxt || (cur == NULL) || (test == NULL))
		return;
	/* if quiet and not SVRL report only failures */
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) && ((ctxt->flags & XML_SCHEMATRON_OUT_XML) == 0) && (test->type == XML_SCHEMATRON_REPORT))
		return;
	if(ctxt->flags & XML_SCHEMATRON_OUT_XML) {
		TODO
	}
	else {
		xmlChar * path;
		char msg[1000];
		long line;
		const xmlChar * report = NULL;
		if(((test->type == XML_SCHEMATRON_REPORT) & (!success)) || ((test->type == XML_SCHEMATRON_ASSERT) & (success)))
			return;
		line = xmlGetLineNo(cur);
		path = xmlGetNodePath(cur);
		SETIFZ(path, (xmlChar*)cur->name);
#if 0
		if((test->report != NULL) && (test->report[0] != 0))
			report = test->report;
#endif
		if(test->node != NULL)
			report = xmlSchematronFormatReport(ctxt, test->node, cur);
		if(report == NULL) {
			if(test->type == XML_SCHEMATRON_ASSERT) {
				report = xmlStrdup((const xmlChar*)"node failed assert");
			}
			else {
				report = xmlStrdup((const xmlChar*)"node failed report");
			}
		}
		snprintf(msg, 999, "%s line %ld: %s\n", (const char*)path, line, (const char*)report);
		if(ctxt->flags & XML_SCHEMATRON_OUT_ERROR) {
			xmlStructuredErrorFunc schannel = NULL;
			xmlGenericErrorFunc channel = NULL;
			void * data = NULL;
			if(ctxt) {
				if(ctxt->serror != NULL)
					schannel = ctxt->serror;
				else
					channel = ctxt->error;
				data = ctxt->userData;
			}
			__xmlRaiseError(schannel, channel, data, NULL, cur, XML_FROM_SCHEMATRONV,
			    (test->type == XML_SCHEMATRON_ASSERT) ? XML_SCHEMATRONV_ASSERT : XML_SCHEMATRONV_REPORT,
			    XML_ERR_ERROR, NULL, line, (pattern == NULL) ? NULL : ((const char*)pattern->name),
			    (const char*)path, (const char*)report, 0, 0, "%s", msg);
		}
		else {
			xmlSchematronReportOutput(ctxt, cur, &msg[0]);
		}
		SAlloc::F((char*)report);
		if((path != NULL) && (path != (xmlChar*)cur->name))
			SAlloc::F(path);
	}
}
/**
 * xmlSchematronReportPattern:
 * @ctxt:  the validation context
 * @pattern: the current pattern
 *
 * called from the validation engine when starting to check a pattern
 */
static void xmlSchematronReportPattern(xmlSchematronValidCtxtPtr ctxt,
    xmlSchematronPatternPtr pattern) {
	if(!ctxt || (pattern == NULL))
		return;
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) || (ctxt->flags & XML_SCHEMATRON_OUT_ERROR)) /* Error gives pattern
		                                                                                   name as part of error
		                                                                                   */
		return;
	if(ctxt->flags & XML_SCHEMATRON_OUT_XML) {
		TODO
	}
	else {
		char msg[1000];

		if(pattern->name == NULL)
			return;
		snprintf(msg, 999, "Pattern: %s\n", (const char*)pattern->name);
		xmlSchematronReportOutput(ctxt, NULL, &msg[0]);
	}
}

/************************************************************************
*									*
*		Validation against a Schematrontron				*
*									*
************************************************************************/

/**
 * xmlSchematronSetValidStructuredErrors:
 * @ctxt:  a Schematron validation context
 * @serror:  the structured error function
 * @ctx: the functions context
 *
 * Set the structured error callback
 */
void xmlSchematronSetValidStructuredErrors(xmlSchematronValidCtxtPtr ctxt,
    xmlStructuredErrorFunc serror, void * ctx)
{
	if(!ctxt)
		return;
	ctxt->serror = serror;
	ctxt->error = NULL;
	ctxt->warning = NULL;
	ctxt->userData = ctx;
}

/**
 * xmlSchematronNewValidCtxt:
 * @schema:  a precompiled XML Schematrons
 * @options: a set of xmlSchematronValidOptions
 *
 * Create an XML Schematrons validation context based on the given schema.
 *
 * Returns the validation context or NULL in case of error
 */
xmlSchematronValidCtxtPtr xmlSchematronNewValidCtxt(xmlSchematronPtr schema, int options)
{
	int i;
	xmlSchematronValidCtxtPtr ret;

	ret = (xmlSchematronValidCtxtPtr)SAlloc::M(sizeof(xmlSchematronValidCtxt));
	if(!ret) {
		xmlSchematronVErrMemory(NULL, "allocating validation context",
		    NULL);
		return 0;
	}
	memzero(ret, sizeof(xmlSchematronValidCtxt));
	ret->type = XML_STRON_CTXT_VALIDATOR;
	ret->schema = schema;
	ret->xctxt = xmlXPathNewContext(NULL);
	ret->flags = options;
	if(ret->xctxt == NULL) {
		xmlSchematronPErrMemory(NULL, "allocating schema parser XPath context",
		    NULL);
		xmlSchematronFreeValidCtxt(ret);
		return 0;
	}
	for(i = 0; i < schema->nbNamespaces; i++) {
		if((schema->namespaces[2 * i] == NULL) ||
		    (schema->namespaces[2 * i + 1] == NULL))
			break;
		xmlXPathRegisterNs(ret->xctxt, schema->namespaces[2 * i + 1],
		    schema->namespaces[2 * i]);
	}
	return ret;
}

/**
 * xmlSchematronFreeValidCtxt:
 * @ctxt:  the schema validation context
 *
 * Free the resources associated to the schema validation context
 */
void xmlSchematronFreeValidCtxt(xmlSchematronValidCtxtPtr ctxt)
{
	if(ctxt) {
		if(ctxt->xctxt != NULL)
			xmlXPathFreeContext(ctxt->xctxt);
		xmlDictFree(ctxt->dict);
		SAlloc::F(ctxt);
	}
}

static xmlNodePtr xmlSchematronNextNode(xmlNodePtr cur)
{
	if(cur->children) {
		/*
		 * Do not descend on entities declarations
		 */
		if(cur->children->type != XML_ENTITY_DECL) {
			cur = cur->children;
			/*
			 * Skip DTDs
			 */
			if(cur->type != XML_DTD_NODE)
				return cur;
		}
	}

	while(cur->next != NULL) {
		cur = cur->next;
		if((cur->type != XML_ENTITY_DECL) &&
		    (cur->type != XML_DTD_NODE))
			return cur;
	}

	do {
		cur = cur->parent;
		if(!cur) break;
		if(cur->type == XML_DOCUMENT_NODE) return 0;
		if(cur->next != NULL) {
			cur = cur->next;
			return cur;
		}
	} while(cur);
	return cur;
}

/**
 * xmlSchematronRunTest:
 * @ctxt:  the schema validation context
 * @test:  the current test
 * @instance:  the document instace tree
 * @cur:  the current node in the instance
 *
 * Validate a rule against a tree instance at a given position
 *
 * Returns 1 in case of success, 0 if error and -1 in case of internal error
 */
static int xmlSchematronRunTest(xmlSchematronValidCtxtPtr ctxt,
    xmlSchematronTestPtr test, xmlDocPtr instance, xmlNodePtr cur, xmlSchematronPatternPtr pattern)
{
	xmlXPathObjectPtr ret;
	int failed;

	failed = 0;
	ctxt->xctxt->doc = instance;
	ctxt->xctxt->node = cur;
	ret = xmlXPathCompiledEval(test->comp, ctxt->xctxt);
	if(!ret) {
		failed = 1;
	}
	else {
		switch(ret->type) {
			case XPATH_XSLT_TREE:
			case XPATH_NODESET:
			    if((ret->nodesetval == NULL) ||
			    (ret->nodesetval->nodeNr == 0))
				    failed = 1;
			    break;
			case XPATH_BOOLEAN:
			    failed = !ret->boolval;
			    break;
			case XPATH_NUMBER:
			    if((xmlXPathIsNaN(ret->floatval)) ||
			    (ret->floatval == 0.0))
				    failed = 1;
			    break;
			case XPATH_STRING:
			    if((ret->stringval == NULL) ||
			    (ret->stringval[0] == 0))
				    failed = 1;
			    break;
			case XPATH_UNDEFINED:
			case XPATH_POINT:
			case XPATH_RANGE:
			case XPATH_LOCATIONSET:
			case XPATH_USERS:
			    failed = 1;
			    break;
		}
		xmlXPathFreeObject(ret);
	}
	if((failed) && (test->type == XML_SCHEMATRON_ASSERT))
		ctxt->nberrors++;
	else if((!failed) && (test->type == XML_SCHEMATRON_REPORT))
		ctxt->nberrors++;

	xmlSchematronReportSuccess(ctxt, test, cur, pattern, !failed);

	return(!failed);
}

/**
 * xmlSchematronValidateDoc:
 * @ctxt:  the schema validation context
 * @instance:  the document instace tree
 *
 * Validate a tree instance against the schematron
 *
 * Returns 0 in case of success, -1 in case of internal error
 *         and an error count otherwise.
 */
int xmlSchematronValidateDoc(xmlSchematronValidCtxtPtr ctxt, xmlDocPtr instance)
{
	xmlNodePtr cur, root;
	xmlSchematronPatternPtr pattern;
	xmlSchematronRulePtr rule;
	xmlSchematronTestPtr test;
	if(!ctxt || (ctxt->schema == NULL) || (ctxt->schema->rules == NULL) || (instance == NULL))
		return -1;
	ctxt->nberrors = 0;
	root = xmlDocGetRootElement(instance);
	if(root == NULL) {
		TODO
		ctxt->nberrors++;
		return 1;
	}
	if((ctxt->flags & XML_SCHEMATRON_OUT_QUIET) || (ctxt->flags == 0)) {
		/*
		 * we are just trying to assert the validity of the document,
		 * speed primes over the output, run in a single pass
		 */
		cur = root;
		while(cur) {
			rule = ctxt->schema->rules;
			while(rule != NULL) {
				if(xmlPatternMatch(rule->pattern, cur) == 1) {
					test = rule->tests;
					while(test != NULL) {
						xmlSchematronRunTest(ctxt, test, instance, cur, (xmlSchematronPatternPtr)rule->pattern);
						test = test->next;
					}
				}
				rule = rule->next;
			}
			cur = xmlSchematronNextNode(cur);
		}
	}
	else {
		/*
		 * Process all contexts one at a time
		 */
		pattern = ctxt->schema->patterns;
		while(pattern != NULL) {
			xmlSchematronReportPattern(ctxt, pattern);
			/*
			 * TODO convert the pattern rule to a direct XPath and
			 * compute directly instead of using the pattern matching over the full document...
			 * Check the exact semantic
			 */
			for(cur = root; cur; cur = xmlSchematronNextNode(cur)) {
				for(rule = pattern->rules; rule; rule = rule->patnext) {
					if(xmlPatternMatch(rule->pattern, cur) == 1) {
						for(test = rule->tests; test; test = test->next)
							xmlSchematronRunTest(ctxt, test, instance, cur, pattern);
					}
				}
			}
			pattern = pattern->next;
		}
	}
	return(ctxt->nberrors);
}

#ifdef STANDALONE
int main()
{
	int ret;
	xmlDocPtr instance;
	xmlSchematronValidCtxtPtr vctxt;
	xmlSchematronPtr schema = NULL;
	xmlSchematronParserCtxtPtr pctxt = xmlSchematronNewParserCtxt("tst.sct");
	if(pctxt == NULL) {
		fprintf(stderr, "failed to build schematron parser\n");
	}
	else {
		schema = xmlSchematronParse(pctxt);
		if(schema == NULL) {
			fprintf(stderr, "failed to compile schematron\n");
		}
		xmlSchematronFreeParserCtxt(pctxt);
	}
	instance = xmlReadFile("tst.sct", NULL, XML_PARSE_NOENT | XML_PARSE_NOCDATA);
	if(instance == NULL) {
		fprintf(stderr, "failed to parse instance\n");
	}
	if((schema != NULL) && (instance != NULL)) {
		vctxt = xmlSchematronNewValidCtxt(schema);
		if(vctxt == NULL) {
			fprintf(stderr, "failed to build schematron validator\n");
		}
		else {
			ret = xmlSchematronValidateDoc(vctxt, instance);
			xmlSchematronFreeValidCtxt(vctxt);
		}
	}
	xmlSchematronFree(schema);
	xmlFreeDoc(instance);
	xmlCleanupParser();
	xmlMemoryDump();
	return 0;
}

#endif
#define bottom_schematron
#include "elfgcchack.h"
#endif /* LIBXML_SCHEMATRON_ENABLED */
