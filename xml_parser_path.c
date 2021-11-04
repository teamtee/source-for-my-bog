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
uint32_t parser_xpath(xmlChar * string,xmlChar * xpath);

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

uint32_t xml_parser_xpath(xmlDocPtr doc,xmlNode** nodetable,xmlChar* xpath);

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
uint32_t get_namenode_in_nodetable(xmlChar *nodename,xmlNode**nodetable)
{
    int nodenumber=0;
    int i;
    int j;
    
    xmlNode** result_nodetable[500];
    xmlNodePtr curnode;
    //get
    for(i =0;nodetable[i]!=NULL;i++)
    {
        printf("this is i:%d\n",i);
        printf("this is nodetable[i]:%s\n",nodetable[i]->name);
        printf("this is the childrenname:%s\n",nodetable[i]->children->name);
        for(j=0;(curnode=get_the_children(j+1,nodetable[i]))!=NULL;j++)
        {
            printf("this is the nodename:%s\n",nodename);
            printf("this is the node_name:%s\n",curnode->name);
            if(curnode->name==nodename)
            {
                nodenumber++;
                result_nodetable[i]=curnode;
            }
        }
    }
    //swap
    nodetable[nodenumber+1]=NULL;
    for(;nodenumber!=0;nodenumber--)
    {
        nodetable[nodenumber]=result_nodetable[nodenumber];
    }
    return nodenumber;
}   
   


/* Parse and validate XML, then apply config */
uint32_t main(uint32_t argc, char **argv)
{   xmlNodePtr curnode;
xmlKeepBlanksDefault(0);
    if (argc < 2)
    {
        printf("No input file.\nUsage: <binary> <file>\n");
        return 1;
    }

    LIBXML_TEST_VERSION

    xmlParserCtxtPtr parser_ctx;
    parser_ctx = xmlNewParserCtxt();
    if (parser_ctx == NULL)
    {
        printf("Failed to allocate parser context.\n");
        return 1;
    }

    doc = xmlCtxtReadFile(parser_ctx,"./maccfg_new.xml", NULL, XML_PARSE_DTDVALID & XML_PARSE_NOBLANKS);
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
    
    //begin
    //change the func
    xmlNodePtr nodetable[500];
    
    xml_parser_xpath(doc,nodetable,XMLPATH_BLOCK_SIZE);
   
}

uint32_t xml_parser_xpath(xmlDocPtr doc,xmlNode** nodetable,xmlChar* xpath){
    xmlChar  node_name[500];
    uint32_t node_number;
    nodetable[0]=xmlDocGetRootElement(doc);
    nodetable[1]=NULL;
    //parser the xpath
    int i=255;
    while(i)
    {
        printf("this is 1\n");
    switch(parser_xpath(node_name,xpath))
    {
        case 0:return node_number;
        case 1:node_number=get_namenode_in_nodetable(node_name,nodetable);printf("this is node_number:%d\n",node_number);break;
        default:
            printf("error in func:xml_parser_xpath");
    }
    i--;
    }
    return 0;
}
uint32_t parser_xpath(xmlChar * string,xmlChar * xpath){
   
    if(*xpath=='/')
    {
        xpath=xpath+1;
    }
    else
    {
        return 0;
    }
    int i;
    for (i=0;(*xpath)!='/'&&(*xpath)!='\0';i++)
    {
        *(string+i)=*xpath;
        *(string+i+1)=0;
        xpath=xpath+1;
    }
    return 1;
    
}
