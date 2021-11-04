#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define PDCP_CIPHER_KEY_LENGTH 16
#define PDCP_INTEGRITY_KEY_LENGTH 16
/* xPath Expr Macros */
#define XMLPATH_MACCFG                      "/MacConfig"
#define XMLPATH_CFG_VERSION                 XMLPATH_MACCFG "/version"
#define XMLPATH_API                         XMLPATH_MACCFG "/Api"
#define XMLPATH_DPDK                        XMLPATH_MACCFG "/DPDK"
#define XMLPATH_DPDK_SHARED                 XMLPATH_MACCFG "/DpdkSharedResource"
#define XMLPATH_THREAD                      XMLPATH_MACCFG "/Threads"
#define XMLPATH_BBUPOOL                     XMLPATH_MACCFG "/BbuPoolConfig"
#define XMLPATH_PDCP                        XMLPATH_MACCFG "/PdcpConfig"
#define XMLPATH_CELL                        XMLPATH_MACCFG "/CellConfig"

#define XMLPATH_WLS_NAME                    XMLPATH_API "/wls_dev_name"
#define XMLPATH_NUM_BLOCK_TYPES             XMLPATH_API "/numBlockTypes"
#define XMLPATH_BLOCK_SIZE                  XMLPATH_API "/blockType/blockSize"
#define XMLPATH_BLOCK_COUNT                 XMLPATH_API "/blockType/blockCount"

#define XMLPATH_DPDK_PREFIX                 XMLPATH_DPDK "/dpdkFilePrefix"
#define XMLPATH_IOVA_MODE                   XMLPATH_DPDK "/dpdkIovaMode"

#define XMLPATH_PCI_F1U_ADDR                XMLPATH_DPDK_SHARED "/PciF1uAddr"
#define XMLPATH_MEMPOOL                     XMLPATH_DPDK_SHARED "/mempoolName"

#define XMLPATH_SYS_THREAD                  XMLPATH_THREAD "/systemThread"
#define XMLPATH_MACIO_THREAD                XMLPATH_THREAD "/macIoThread"
#define XMLPATH_URLLC_THREAD                XMLPATH_THREAD "/urllcThread"
#define XMLPATH_SYSTEM_CORE                 XMLPATH_SYS_THREAD "/core"
#define XMLPATH_SYSTEM_PRIORITY             XMLPATH_SYS_THREAD "/priority"
#define XMLPATH_SYSTEM_POLICY               XMLPATH_SYS_THREAD "/policy"
#define XMLPATH_MACIO_CORE                  XMLPATH_MACIO_THREAD "/core"
#define XMLPATH_MACIO_PRIORITY              XMLPATH_MACIO_THREAD "/priority"
#define XMLPATH_MACIO_POLICY                XMLPATH_MACIO_THREAD "/policy"
#define XMLPATH_URLLC_CORE                  XMLPATH_URLLC_THREAD "/core"
#define XMLPATH_URLLC_PRIORITY              XMLPATH_URLLC_THREAD "/priority"
#define XMLPATH_URLLC_POLICY                XMLPATH_URLLC_THREAD "/policy"

#define XMLPATH_BBUPOOL_SLEEP               XMLPATH_BBUPOOL "/BbuPoolSleepEnable"
#define XMLPATH_BBUPOOL_PRIORITY            XMLPATH_BBUPOOL "/BbuPoolThreadCorePriority"
#define XMLPATH_BBUPOOL_POLICY              XMLPATH_BBUPOOL "/BbuPoolThreadCorePolicy"
#define XMLPATH_BBUPOOL_COREMASK            XMLPATH_BBUPOOL "/BbuPoolThreadCoreMask"

#define XMLPATH_SNBIT                       XMLPATH_PDCP "/snBit"
#define XMLPATH_DISCARD_T                   XMLPATH_PDCP "/discardTimer"
#define XMLPATH_OUT_OF_ORDER_DELIV          XMLPATH_PDCP "/outOfOrderDelivery"
#define XMLPATH_INT_CFG                     XMLPATH_PDCP "/integerityConfigured"
#define XMLPATH_CRYPTO_MODE                 XMLPATH_PDCP "/cryptoMode"
#define XMLPATH_CIPH_ALGO                   XMLPATH_PDCP "/ciphAlgo"
#define XMLPATH_INT_ALGO                    XMLPATH_PDCP "/intAlgo"
#define XMLPATH_CIPH_KEY                    XMLPATH_PDCP "/ciphKey"
#define XMLPATH_INT_KEY                     XMLPATH_PDCP "/intKey"

#define XMLPATH_NUM_CELL                    XMLPATH_CELL "/numCell"
#define XMLPATH_CELL_CFG_FILE               XMLPATH_CELL "/cellConfigFile"


typedef enum _ClineType
{
    CLINE_TYPE_INVALID,
    CLINE_TYPE_UINT32,
    CLINE_TYPE_UINT64,
    CLINE_TYPE_STR,
    CLINE_TYPE_UINT8_ARRAY,
    CLINE_TYPE_MAX
} ClineType;

static xmlDocPtr doc;
static xmlXPathObjectPtr curObj;
static xmlXPathContextPtr curCtx;
static char xmlVersion[256];


/* Convert str(hex) to uint8 and apply to dest. Returns success=0, 
failure=1. */
uint32_t xml_apply_uint8(char *string, uint8_t *dest)
{
    uint32_t value = strtoul(string, NULL, 16);
    if ((value == 0 && errno == EINVAL) || (value == INT_MAX && errno == ERANGE))
    {
        printf("xml_apply_uint8 failed.\n");
        return 1;
    }
    if (value > 255)
    {
        printf("\"%s\" out of range.", string);
        return 1;
    }
    *dest = value;
    return 0;
}

/* Convert str(oct) to uint32 and apply to dest. Returns success=0, 
failure=1. */
uint32_t xml_apply_uint32(char *string, uint32_t *dest)
{
    
    uint32_t value = strtoul(string, NULL, 0);

    if ((value == 0 && errno == EINVAL) || (value == INT_MAX && errno == ERANGE))
    {
        printf("xml_apply_uint32 failed.\n");
        return 1;
    }
    
    *dest = value;
    
    return 0;
}

/* Convert str(hex) to uint64 and apply to dest. Returns success=0, 
failure=1. */
uint32_t xml_apply_uint64(char *string, uint64_t *dest)
{
    uint64_t value = strtoull(string, NULL, 16);
    if ((value == 0 && errno == EINVAL) || (value == INT_MAX && errno == ERANGE))
    {
        printf("xml_apply_uint64 failed.\n");
        return 1;
    }
    *dest = value;
    return 0;
}
/*function for xml_xpath_find()*/
int xml_node_latitude_examine(xmlNodePtr rootnode){
    xmlChar* text="text";
    rootnode=rootnode->parent;
    if(rootnode->children==NULL||(!xmlStrcmp(rootnode->children->name,text)))
    {
        return 0;
    }
    xmlNodePtr curnode=rootnode->children;
    int latitude=1;
    while(curnode->next!=NULL){
        curnode=curnode->next;
        latitude++;
    }
    return latitude;
}
xmlNodePtr get_the_children(int num,xmlNodePtr curnode){
    curnode=curnode->children;
    if(!curnode){
        printf("get_the_children():no children node");
        return NULL;
    }
    while(num-1){
            curnode=curnode->next;
            num--;
    }
    return curnode;
}
    
   


/* Parse and validate XML, then apply config */
uint32_t main(uint32_t argc, char **argv)

{   xmlNodePtr curnode;
    if (argc < 2)
    {
        printf("No input file.\nUsage: <binary> <file>\n");
        return 1;
    }

    LIBXML_TEST_VERSION
    xmlKeepBlanksDefault(0);
    xmlParserCtxtPtr parser_ctx;
    parser_ctx = xmlNewParserCtxt();
    if (parser_ctx == NULL)
    {
        printf("Failed to allocate parser context.\n");
        return 1;
    }

    doc = xmlCtxtReadFile(parser_ctx, argv[1], NULL, XML_PARSE_DTDVALID & XML_PARSE_NOBLANKS);
    if (doc == NULL)
    {
        printf("Failed to parse \"%s\".\n", argv[1]);
        return 1;
    }
    else if (parser_ctx->valid == 0)
    {
        printf("Failed to validate \"%s\"\n", argv[1]);
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(parser_ctx);
        xmlCleanupParser();
        return 1;
    }
    printf("MacCfg read and validation successful.\n");
    uint32_t dest[4];
    //begin
    xmlXPathObjectPtr curObj;
    xmlXPathContextPtr curCtx;
    curCtx = xmlXPathNewContext(doc);
    curObj = xmlXPathEval(XMLPATH_API"/blockType", curCtx);
    xmlNodeSetPtr curNodeSet= curObj->nodesetval;
    //to creat the dataset
    curnode=curNodeSet->nodeTab[0];
    
    
    int i;
    for ( i = 0; i < curNodeSet->nodeNr; i++)
    {
        printf("**********************************************************\n",i);
        printf("this is the %d node\n",i);
        printf("this is the name :%s\n",curNodeSet->nodeTab[i]->name);
        printf("this is the text :%s\n",curNodeSet->nodeTab[i]->content);
        printf("this is the one childname :%s\n",curNodeSet->nodeTab[i]->children->name);
        printf("this is the one childtext :%s\n",curNodeSet->nodeTab[i]->children->content);
        /*
        printf("this is the two childname :%s\n",curNodeSet->nodeTab[i]->next->next->name);
        printf("this is the two childtext :%s\n",curNodeSet->nodeTab[i]->next->next->content);
        */
        printf("this is the three childname :%s\n",curNodeSet->nodeTab[i]->children->next->next->name);
        printf("this is the three childtext :%s\n",curNodeSet->nodeTab[i]->children->next->next->content);

        
        

        /*
        printf("this is the fathername: %s\n",curNodeSet->nodeTab[i]->parent->name);
        printf("this is the father-children:name: %s\n",curNodeSet->nodeTab[i]->parent->children->name);
        printf("this is the one father-nextname: %s\n",curNodeSet->nodeTab[i]->parent->next->name);
        printf("this is the two father-nextname: %s\n",curNodeSet->nodeTab[i]->parent->next->next->name);
        */
        //printf("this is the fathername: %s\n",curNodeSet->nodeTab[i]);
    }
    
    
    xmlXPathFreeContext(curCtx);
    xmlXPathFreeObject(curObj);    
}
