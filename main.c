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

/* Shortcut for Readability */
typedef struct __CLINE_CONFIG_ITEM CLINE_CONFIG_ITEM;

/* 
 *  @param  config_item     A pointer of CLINE_CONFIG_ITEM
 *  @return                 All members of CLINE_CONFIG_ITEM
 */
#define ARGUMENTS_OF_CONFIG_ITEM(config_item)   \
    (config_item)->name, (config_item)->dest, (config_item)->xPathExpr, (config_item)->type, \
    (config_item)->length, (config_item)->num, (config_item)->alt_num, (config_item)->deflt

typedef struct tBbuPoolInfo
{
    uint32_t sleepEnable;
    uint32_t corePriority;
    uint32_t corePolicy;
    uint64_t coreMask[4];
} TBBUPOOL_INFO, *PBBUPOOL_INFO;

typedef struct tThreadInfo
{
    uint32_t coreId;
    uint32_t corePriority;
    uint32_t corePolicy;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct tPdcpCfg
{
    uint8_t snBit;
    uint32_t outOfOrderDelivery;   // 0: in sequence , 1: out of order
    uint32_t integerityConfigured; // 0: not configured, 1: configured
    uint8_t cryptoMode;            // valid value : 0: SW,  1: HW
    uint8_t ciphAlgo;              // valid value : 1:NULL  6:AES_CTR   12:SNOW3G  13:ZUC
    uint8_t intAlgo;               // valid value : 1:NULL  3:AES_CMAC  19:SNOW3G  20:ZUC
    uint64_t discardTimer;         // time in ms
    uint8_t ciphKey[16];
    uint8_t intKey[16];
} PDCP_CFG, *PPDCP_CFG;

typedef struct tWlsCfgInfo
{
    char wls_dev_filename[512];
    uint32_t numBlockTypes;
    uint32_t blockSize[6];
    uint32_t blockCount[6];
} WLSCFG_INFO, *PWLSCFG_INFO;

typedef struct tMacCfgVars
{
    char cfg_filename[256];

    WLSCFG_INFO wlsCfg;
    uint32_t dpdkIoVaMode;
    char dpdkFilePrefix[256];

    uint32_t numCell;
    char cellFileNameArr[12][256];

    THREAD_INFO macIoThread;
    THREAD_INFO systemThread;
    THREAD_INFO urllcThread;
    TBBUPOOL_INFO bbuPoolInfo;

    char f1u_pcie_addr[2][32];
    char f1u_pool_name[2][32];

    PDCP_CFG pdcpCfg;
} MACCFG_VARS, *PMACCFG_VARS;

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
static MACCFG_VARS gCfgVars;
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
uint32_t xml_xpath_find(xmlDocPtr doc,void *dest, xmlChar *xPath, ClineType type)
{   //read the nodeset
    xmlXPathObjectPtr curObj;
    xmlXPathContextPtr curCtx;
    xmlNodePtr curnode;
    //for dateset
    int dataset_latitude_array[4];
    int dataset_latitude;
    int i,j,k,l=0;
    //temp
    if (doc == NULL){
        printf("There is no parsing result.\n");
        return 0;
    }
    curCtx = xmlXPathNewContext(doc);
    if (curCtx == NULL){
        printf("Unable to create new XPath context.\n");
        return 0;
    }
    curObj = xmlXPathEval(xPath, curCtx);
    if (curObj == NULL){
        printf("Unable to eval xpath expression \"%s\".\n", xPath);
        return 0;
    }
    if (xmlXPathNodeSetIsEmpty(curObj->nodesetval)){
        printf ("Unable to find the xpath aims\"%s\".\n",xPath);
    }
    
    xmlNodeSetPtr curNodeSet= curObj->nodesetval;
    //to creat the dataset
    dataset_latitude_array[0] = curNodeSet->nodeNr;
    dataset_latitude=1;
    curnode=curNodeSet->nodeTab[0];
    for(i=0;dataset_latitude_array[i+1]=xml_node_latitude_examine(curnode);i++){
        curnode=curnode->children;
        dataset_latitude++;
    }
    if(dataset_latitude==1&&dataset_latitude_array[0]==1&&type!=CLINE_TYPE_STR)
    {
        curnode=curNodeSet->nodeTab[0];
        xmlChar string[500];
        for ( i = 0; *(curnode->children->content+i)!='\0'; i++)
        {
            string[i]=*(curnode->children->content+i);
        }
        string[i]='\0';
        for(i=0,j=0;1;i++)
        {
            if(string[i]=='\0')
            {
                xmlXPathFreeContext(curCtx);
                xmlXPathFreeObject(curObj);
                return 0;           
                }
            else if (string[i]==','||i==0)
            {
                if(i==0)
                {
                    i--;
                }
                if(type==CLINE_TYPE_STR){
                break;
                }
                else if (type==CLINE_TYPE_UINT8_ARRAY){
                xml_apply_uint8(curnode->children->content+i+1,dest+sizeof(uint8_t)*j);
                }
                else if(type==CLINE_TYPE_UINT32)
                {
                    xml_apply_uint32(curnode->children->content+i+1,dest+sizeof(uint32_t)*j);  
                }
                else if (type==CLINE_TYPE_UINT64)
                {   
                    xml_apply_uint64(curnode->children->content+i+1,dest+sizeof(uint64_t)*j);
                }
                else{
                    printf("error type");
                    return 1;
                }
                j++;
            }
            else
            {
                ;
            }
            if((i+1)==0)
                {
                    i++;
                }
        }
    }
    if(dataset_latitude==1){
        
        xmlNodePtr curnode1;
        for(i=0;i<dataset_latitude_array[0];i++){
            curnode1=curNodeSet->nodeTab[i];
            if(type==CLINE_TYPE_STR){
                *((xmlChar**)(dest+sizeof(xmlChar)*i))=curnode1->children->content;
            }
            else if (type==CLINE_TYPE_UINT8_ARRAY){
                xml_apply_uint8(curnode1->children->content, dest+sizeof(uint8_t)*i);
            }
            else if(type==CLINE_TYPE_UINT32){
                xml_apply_uint32(curnode1->children->content, dest+sizeof(uint32_t)*i);
                
            }
            else if (type==CLINE_TYPE_UINT64)
            {
                xml_apply_uint64(curnode1->children->content, dest+sizeof(uint64_t)*i);
            }
            else{
                printf("error type");
                return 1;
            }
            
            
        }
    }
    else if(dataset_latitude==2){
        
        xmlNodePtr curnode1;
        xmlNodePtr curnode2;
        
        for(i=0;i<dataset_latitude_array[0];i++){
            
            curnode1=curNodeSet->nodeTab[i];
            curnode2=curnode1->children;
            for( j=0;j<dataset_latitude_array[1];j++){
                if(type==CLINE_TYPE_STR){
                *((xmlChar**)(dest+sizeof(xmlChar)*i*dataset_latitude_array[1]+sizeof(xmlChar)*j))=curnode2->children->content;
            }
            else if (type==CLINE_TYPE_UINT8_ARRAY){
                xml_apply_uint8(curnode2->children->content, dest+sizeof(uint8_t)*i*dataset_latitude_array[1]+sizeof(uint8_t)*j);
            }
            else if(type==CLINE_TYPE_UINT32){
                xml_apply_uint32(curnode2->children->content,(uint32_t* )(dest+sizeof(uint32_t)*i*dataset_latitude_array[1]+sizeof(uint32_t)*j) );
                
               
            }
            else if (type==CLINE_TYPE_UINT64)
            {
                xml_apply_uint64(curnode2->children->content,dest+sizeof(uint64_t)*i*dataset_latitude_array[1]+sizeof(uint64_t)*j);
            }
            else{
                printf("error type");
                return 1;
            }
                curnode2=curnode2->next;
            }  
        }
    }
    else if(dataset_latitude==3){
        xmlNodePtr curnode1;
        xmlNodePtr curnode2;
        xmlNodePtr curnode3;
        for(i=0;i<dataset_latitude_array[0];i++){
            curnode1=curNodeSet->nodeTab[i];
            for(j=0;j<dataset_latitude_array[1];j++){
                curnode2=get_the_children(j,curnode1);
                for(k=0;j<dataset_latitude_array[2];j++){
                    curnode3=get_the_children(k,curnode2);
                    if(type==CLINE_TYPE_STR){
                *((xmlChar**)(dest+sizeof(xmlChar)*i*dataset_latitude_array[1]*dataset_latitude_array[2]+sizeof(xmlChar)*j*dataset_latitude_array[2]+sizeof(xmlChar)*k))=curnode3->children->content;
            }
            else if (type==CLINE_TYPE_UINT8_ARRAY){
                xml_apply_uint8(curnode3->children->content, dest+sizeof(uint8_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]+sizeof(uint8_t)*dataset_latitude_array[2]*j+sizeof(uint8_t)*k);
            }
            else if(type==CLINE_TYPE_UINT32){
                xml_apply_uint32(curnode3->children->content,dest+sizeof(uint32_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]+sizeof(uint32_t)*dataset_latitude_array[2]*j+sizeof(uint32_t)*k);
            }
            else if (type==CLINE_TYPE_UINT64)
            {
                xml_apply_uint64(curnode3->children->content,dest+sizeof(uint64_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]+sizeof(uint64_t)*dataset_latitude_array[2]*j+sizeof(uint64_t)*k);
            }
            else{
                printf("error type");
                return 1;
            }
                }
            }
            
        }
    }
    else if(dataset_latitude==4){
        xmlNodePtr curnode1;
        xmlNodePtr curnode2;
        xmlNodePtr curnode3;
        xmlNodePtr curnode4;
        for(i=0;i<dataset_latitude_array[0];i++){
            curnode1=curNodeSet->nodeTab[i];
            for(j=0;j<dataset_latitude_array[1];j++){
                curnode2=get_the_children(j,curnode1);
                for(k=0;k<dataset_latitude_array[2];k++){
                    curnode3=get_the_children(k,curnode2);
                    for(l=0;l<dataset_latitude_array[3];l++){
                        curnode4=get_the_children(l,curnode3);
                        if(type==CLINE_TYPE_STR){
                *((xmlChar**)(dest+sizeof(xmlChar)*i*dataset_latitude_array[1]*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(xmlChar)*j*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(xmlChar)*k*dataset_latitude_array[3]+sizeof(xmlChar)*l))=curnode4->children->content;
            }
            else if (type==CLINE_TYPE_UINT8_ARRAY){
                xml_apply_uint8(curnode4->children->content, dest+sizeof(uint8_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint8_t)*j*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint8_t)*k*dataset_latitude_array[3]+sizeof(uint8_t)*l);
            }
            else if(type==CLINE_TYPE_UINT32){
                xml_apply_uint32(curnode4->children->content, dest+sizeof(uint32_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint32_t)*j*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint32_t)*k*dataset_latitude_array[3]+sizeof(uint32_t)*l);
            }
            else if (type==CLINE_TYPE_UINT64)
            {
                xml_apply_uint64(curnode4->children->content,dest+sizeof(uint64_t)*i*dataset_latitude_array[1]*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint64_t)*j*dataset_latitude_array[2]*dataset_latitude_array[3]+sizeof(uint64_t)*k*dataset_latitude_array[3]+sizeof(uint64_t)*l);
            }
            else{
                printf("error type");
                return 1;
            }
                    }
                    
                }
            }  
        }
    }
    else{
        printf("latitude out of range");
        return 0;
    }
    //free
    
    xmlXPathFreeContext(curCtx);
    xmlXPathFreeObject(curObj);
    return 1;
}


/* Parse and validate XML, then apply config */
uint32_t main(uint32_t argc, char **argv)
{
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
    uint64_t dest[50];
    int i;
    xml_xpath_find(doc,dest,XMLPATH_CIPH_KEY,CLINE_TYPE_UINT64);
}
