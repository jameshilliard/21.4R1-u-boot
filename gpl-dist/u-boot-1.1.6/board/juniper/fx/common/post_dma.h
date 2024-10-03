
#ifndef POST_DMA_H
#define POST_DMA_H


#define COP_0_INDEX        $0

#define RMI_MAX_NAME_SIZE 32
extern struct bucket_size xls_bucket_sizes;
extern struct stn_cc xls_cc_table_cpu_0;

static struct rmi_dma rmiboard;

enum  dma_config_reg {
  DMA0_BUCKET_SIZE              = 0x320,
  DMA1_BUCKET_SIZE,
  DMA2_BUCKET_SIZE,
  DMA3_BUCKET_SIZE,
};

#define MSGRNG_STNID_DMA      104  
#define MSGRNG_STNID_DMA_0    104
#define MSGRNG_STNID_DMA_1    105
#define MSGRNG_STNID_DMA_2    106
#define MSGRNG_STNID_DMA_3    107


#define DMA_CC_CPU0_0      0x380
#define NR_CPUS 32
#define DMA_DFLT_FIFO_SIZE 4

enum dma_types {
  SIMPLE_XFER = 0,
  CSUM,
  CRC,
  CSUM_SIMPLE_XFER,
  CRC_SIMPLE_XFER,
  CRC_CSUM,
  CRC_CSUM_SIMPLE_XFER,
  MAX_DMA_TYPES
};

#define DMA_CONTROL_CHANNEL0 0x08
#define DMA_CONTROL_CHANNEL1 0x09
#define DMA_CONTROL_CHANNEL2 0x0a
#define DMA_CONTROL_CHANNEL3 0x0b

#define DMA_BUF_LENGTH (0xfffff/8)

static unsigned char *buf1[NR_CPUS];
static unsigned char *buf2[NR_CPUS];

struct dma_config {
  int msg_type;
  int cache_w_coh;
  int cache_r_coh;
  int l2_alloc;
  int use_old_crc;
  int inform_src;
  int init_csum;
  int init_crc;
  int tid;
};


#define MSGRNG_CODE_DMA 8
#define MAX_MSGSND_TRIES 16

struct dma_config xlr_dmacfg[NR_CPUS][MAX_DMA_TYPES];

struct rmi_chip_family {
        int xlr;                        // is xlr processor family
        int xls;                        // is xls processor family
        char family_name[RMI_MAX_NAME_SIZE];
};


typedef struct function
{
        char *func_name;
        /*contains some description about each function*/
        char *description;
         /* keeps a track of the argument count */
        int argument_count;
        /* indexes the 'arg' structure */
        int argument_detail_pointer;
        /* gives index to the row containing the next alternate usage
         * of a particular function */
        int next_alternate_pointer;

} Function_struct;

typedef struct argument
{
        char *argument_name;
        /* indicates the data type of the argument */
        char *argument_type;
        /*indicates whether the argument is mandatory,optional or fixed*/
        char * category;
        /* indicates the argument's default values */
        char * value;
        /* indexes the 'range' structure */
        int range_pointer;

} Argument_struct;

typedef struct value_range
{
        char * value;

} Range_struct;


typedef struct  ph_cpregister_s {
        unsigned char width;
        unsigned char no_of_fields;
        unsigned char reg_num_list[17];
        unsigned char selection_list[17];
        unsigned char access:2;
        unsigned int reg_type:20;
        char *name;
        char *description;
/*} ph_cpregister __attribute__ ((__packed__));*/
} __attribute__ ((__packed__)) ph_cpregister ;

typedef struct cpreg_field_s {
        unsigned char access:2;
        unsigned char no_of_bits;
        char *name;
        char *description;
/*} cpreg_field __attribute__ ((__packed__));*/
} __attribute__ ((__packed__)) cpreg_field ;


typedef struct cpu_s {
  unsigned short no_of_regs;
  ph_cpregister *cpu_reg;
  cpreg_field *cpu_fields;
  char *name;
} __attribute__ ((__packed__)) cpuT ;


typedef struct  ph_register_s {
        unsigned char width;
        unsigned short index;
        unsigned char no_of_fields;
        int default_val;
        unsigned char access:2;
        unsigned int reg_type:20;
        char *name;
        char *description;
} __attribute__ ((__packed__)) ph_register;

typedef struct reg_field_s {
        unsigned char access:2;
        unsigned char no_of_bits;
        char *name;
        char *description;
} __attribute__ ((__packed__)) reg_field ;

typedef struct device_s {
        int offset;
        unsigned short no_of_regs;
        ph_register *device_reg;
        reg_field *device_fields;
        char *name;
} __attribute__ ((__packed__)) deviceT ;

struct rmi_chip {
        int chip_id;                    // stores cop0 prid [0:24]
        int xlr_type;                   // xlr processor type. = 0 for xls.
        int xls_type;                   // xls processor type. = 0 for xls.
        char name[RMI_MAX_NAME_SIZE];   // name of chip
        char revision[RMI_MAX_NAME_SIZE];       // revison details
        struct rmi_chip_family rmichipfamily;
        int total_cores;                // total number of cpu cores
        int total_threads_per_core;     // h/w threads per core
        int total_vcpu;                 // total number of h/w threads in system
        uint32_t thread_wakeup_mask;    // default wakeup mask
        uint32_t vcpu0_thread_mask;     // cpu0 wakeup mask useful for fmn
        int l1_dcache_present;          // flag indicating if L1 dcache present
        int l1_dcache_size;             // L1 data cache size
        int l1_dcacheline_size;         // L1 data cache line size
        int l1_dcache_num_ways;         // L1 data cache ways
        int l1_dcache_ecc_enabled;      // L1 data cache ECC protection enabled
        int l1_icache_present;          // flag indicating if L1 icache present
        int l1_icache_size;             // L1 instruction cache size
        int l1_icacheline_size;         // L1 instruction cache line size
        int l1_icache_num_ways;         // L1 instruction cache ways
        int l1_icache_ecc_enabled;      // L1 instruction cache ECC protection enabled
        int l2_cache_present;           // flag indicating if L2 cache present
        int l2_cache_size;              // L2 cache size
        int l2_cacheline_size;          // L2 cache line size
        int l2_cache_num_ways;          // L2 cache ways
        int l2_cache_ecc_enabled;       // L2 cache ECC protection enabled
        volatile uint32_t *l2_mmio;     // L2 cache i/o address
        int l3_cache_present;           // flag indicating if L3 cache present
        int l3_cache_size;              // L3 cache size
        int l3_cacheline_size;          // L3 cache line size
        int l3_cache_num_ways;          // L3 cache ways
        int l3_cache_ecc_enabled;       // L3 cache ECC protection enabled
        volatile uint32_t *l3_mmio;     // L3 cache i/o address
        int bridge_present;             // flag indicating if system bridge present
        volatile uint32_t *bridge_mmio; // bridge i/o address
        int ddr_ctrl_present;           // flag indicating if ddr1/ddr2/ddr3 controller is present
        int ddr_ctrl_num;               // number of DDR controllers
        volatile uint32_t *ddr2_c0_mmio;        // DDR2 channel0 i/o address
        volatile uint32_t *ddr2_c1_mmio;        // DDR2 channel1 i/o address
        volatile uint32_t *ddr2_c2_mmio;        // DDR2 channel2 i/o address
        volatile uint32_t *ddr2_c3_mmio;        // DDR2 channel3 i/o address
        int sram_ctrl_present;          // flag indicating if SRAM controller present
        int sram_ctrl_num;              // number of sram controllers
        volatile uint32_t *sram_mmio;   // sram i/o address
        struct bucket_size *rmi_bucket_sizes;   // FMN bucket sizes table ptr
        struct stn_cc *rmi_cc_table_cpu_0;      // FMN cpu 0 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_1;      //
        struct stn_cc *rmi_cc_table_cpu_2;      // FMN cpu 2 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_3;      // FMN cpu 3 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_4;      // FMN cpu 4 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_5;      // FMN cpu 5 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_6;      // FMN cpu 6 credit table ptr
        struct stn_cc *rmi_cc_table_cpu_7;      // FMN cpu 7 credit table ptr
        struct stn_cc *rmi_cc_table_xgs_0;      // FMN xgs 0 credit table ptr
        struct stn_cc *rmi_cc_table_xgs_1;      // FMN xgs 1 credit table ptr
        struct stn_cc *rmi_cc_table_gmac_0;     // FMN gmac 0 credit table ptr
        struct stn_cc *rmi_cc_table_gmac_1;     // FMN gmac 1 credit table ptr
        struct stn_cc *rmi_cc_table_dma;        // FMN dma credit table ptr
        struct stn_cc *rmi_cc_table_cmp;        // FMN cmp credit table ptr
        struct stn_cc *rmi_cc_table_pcie;       // FMN pcie credit table ptr
        struct stn_cc *rmi_cc_table_sec;        // FMN sec credit table ptr
        int pic_present;                // flag indicating if PIC present
        int pic_num_irt;                // number of IRT entries in PIC
        volatile uint32_t *pic_mmio;    // PIC i/o address
        int gmac_present;               // flag indicating if gmac's are present
        int gmac_ctrl_num;              // number of gmac controllers
        int gmac_num;                   // number of gmac's
        uint32_t port_mask;             // port mask useful for phy programming
        int max_gmac_bandwidth;         // max gmac traffic possible with chip
        volatile uint32_t *gmac0_mmio;  // gmac0 i/o address
        int gmac0_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac1_mmio;  // gmac1 i/o address
        int gmac1_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac2_mmio;  // gmac2 i/o address
        int gmac2_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac3_mmio;  // gmac3 i/o address
        int gmac3_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac4_mmio;  // gmac4 i/o address
        int gmac4_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac5_mmio;  // gmac5 i/o address
        int gmac5_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac6_mmio;  // gmac6 i/o address
        int gmac6_mode;                 // whether rgmii or sgmii
        volatile uint32_t *gmac7_mmio;  // gmac7 i/o address
        int gmac7_mode;                 // whether rgmii or sgmii
        int xgmac_present;              // flag indicating if xgmacs are present
        int xgmac_ctrl_num;             // number of xgmac controllers
        int xgmac_num;                  // number of xgmac ports
        int max_xgmac_bandwidth;        // max xgmac traffic possible with chip
        volatile uint32_t *xgmac0_mmio; // xgmac0 i/o address
       volatile uint32_t *xgmac1_mmio; // xgmac1 i/o address
        int spi4_present;               // flag indicating if spi4 is present
        int spi4_ctrl_num;              // number of spi4 controllers
        int spi4_num;                   // number of spi4 ports
        int max_spi4_bandwidth;         // max spi4 traffic possible with chip
        volatile uint32_t *spi4_0_mmio; // spi4_0 i/o address
        volatile uint32_t *spi4_1_mmio; // spi4_1 i/o address
        int sec_present;                // flag indicating if security is present
        int sec_ctrl_num;               // number of sec controllers
        int sec_cores;                  // total number of sec cores
        int rsa_cores;                  // total number of sec rsa cores
        int max_sec_bandwidth;          // max security accel possible with chip
        int sec_rsa_stid;               // RSA station id
        volatile uint32_t *sec_mmio;    // sec i/o address
        int pcix_present;               // flag indicating if PCIX is present
        int pcix_ctrl_num;              // number of pcix controllers
        volatile uint32_t *pcix_mmio;   // pcix i/o address
        int ht_present;                 // flag indicating if HT is present
        int ht_ctrl_num;                // number of HT controllers
        volatile uint32_t *ht_mmio;     // ht i/o address
        int pcie_present;               // flag indicating if PCIe is present
        int pcie_ctrl_num;              // number of pcie controllers
        int pcie_type;                  // whether pcie controller configured in by-1, by-2 or by-4 modes
        volatile uint32_t *pcie0_mmio;  // pcie0 i/o address
        volatile uint32_t *pcie1_mmio;  // pcie1 i/o address
        int boot_flash_present;         // flag indicating if boot flash is present
        int boot_flash_ctrl_num;        // number of boot flash controllers
        volatile uint32_t *flash_mmio;  // flash i/o address
        int nand_flash_present;         // flag indicating if NAND flash is present
        int nand_flash_ctrl_num;        // number of NAND flash controllers
        volatile uint32_t *nand_flash_mmio;     // nandflash i/o address
        int dma_present;                // flag indicating if dma is present
        int dma_ctrl_num;               // number of dma controllers
        volatile uint32_t *dma_mmio;    // dma i/o address
        int cmp_present;                // flag indicating if cmp is present
        int cmp_ctrl_num;               // number of cmp controllers
        int max_cmp_bandwidth;          // max cmp accel possible with chip
        volatile uint32_t *cmp_mmio;    // cmp i/o address
        int usb_present;                // flag indicating if USB is present
        int usb_ctrl_num;               // number of usb controllers
        int usb_num_ports;              // number of usb ports. e.g XLS has 2
        volatile uint32_t *usb0_mmio;   // usb i/o address
        volatile uint32_t *usb1_mmio;   // usb i/o address
        int uart_present;               // flag indicating if uart is present
        int uart_ctrl_num;              // number of uart controllers
        volatile uint32_t *uart0_mmio;  // uart0 i/o address
        volatile uint32_t *uart1_mmio;  // uart1 i/o address
        int i2c_present;                // flag indicating if i2c is present
        int i2c_ctrl_num;               // number of i2c controllers
        volatile uint32_t *i2c0_mmio;   // i2c0 i/o address
        volatile uint32_t *i2c1_mmio;   // i2c1 i/o address
        int gpio_present;               // flag indicating if gpio is present
        int gpio_ctrl_num;              // number of gpio controllers
        volatile uint32_t *gpio_mmio;   // gpio i/o address
        int tb_present;                 // flag indicating if trace buffer is present
        int tb_ctrl_num;                // number of trace buffer controllers
        volatile uint32_t *tb_mmio;     // trace buffer i/o address
        Function_struct *rmi_cmd_funcs_list;
        Argument_struct *rmi_cmd_args_list;
        Range_struct *rmi_args_range_list;
        cpuT *rmi_reg_specs_cpus_list;
        deviceT *rmi_reg_specs_device_list;
        deviceT *rmi_reg_specs_control_list;
        int rmi_reg_specs_num_cpu;
        int rmi_reg_specs_num_devices;
        int rmi_regs_specs_num_control;

        void (*chip_init)(struct rmi_chip *chip);
        void (*chip_reset)(struct rmi_chip *chip);
        void (*chip_fmn_init)(struct rmi_chip *chip);
        void (*park_chip)(struct rmi_chip *chip);
        void (*chip_wakeup)(struct rmi_chip *chip);
};


struct rmi_dma {
        int major_num;                  // board major num
        int minor_num;                  // board minor num
        char name[RMI_MAX_NAME_SIZE];   // board name
        struct rmi_chip rmichip;                // rmi chip on board
        int ddr_present;                // flag indicating if ddr1/ddr2/ddr3 RAM present
        int rld_present;                // flag indicating if RLD RAM present
        int sram_present;               // flag indicating if SRAM present
        int uart_num;                   // number of uart's on board
        int gmac0_mode;                 // defines if gmac0 is sgmii=0 / rgmii=1
        int phy_addr_0;                 // PHY address offset for gmac 0
        int phy_addr_1;                 // PHY address offset for gmac 1
        int phy_addr_2;                 // PHY address offset for gmac 2
        int phy_addr_3;                 // PHY address offset for gmac 3
        int phy_addr_4;                 // PHY address offset for gmac 4
        int phy_addr_5;                 // PHY address offset for gmac 5
        int phy_addr_6;                 // PHY address offset for gmac 6
        int phy_addr_7;                 // PHY address offset for gmac 7
        volatile uint32_t *phy_mmio;    // PHY mmio address offset
        int phy_num;                    // Number of PHYs

        void (*board_init)(struct rmi_dma *board);
        void (*board_reset)(struct rmi_dma *board);
};


static __inline__ void dma_write_reg(uint32_t reg, uint32_t val)
{
  phoenix_reg_t *mmio = rmiboard.rmichip.dma_mmio;
  phoenix_write_reg(mmio, reg, val);
}

static __inline__ uint32_t dma_read_reg(uint32_t reg)
{
  phoenix_reg_t *mmio = rmiboard.rmichip.dma_mmio;
  return phoenix_read_reg(mmio, reg);
}


#define PHOENIX_IO_DMA_OFFSET             0x1A000


#define MSGRNG_MSG_STATUS_REG_JACK $2

#define read_32bit_cp2_register_jack(source)                         \
({ int __res;                                                   \
        __asm__ __volatile__(                                   \
        ".set\tpush\n\t"                                        \
        ".set\treorder\n\t"                                     \
        "mfc2\t%0,"STR(source)"\n\t"                            \
        ".set\tpop"                                             \
        : "=r" (__res));                                        \
        __res;})


static __inline__ struct dma_config *xlr_dma_config(int msg_type, int cache_w_coh,
                   int cache_r_coh, int l2_alloc,
                   int use_old_crc, int inform_src,
                   int init_csum, int init_crc)
{
    struct dma_config *dmacfg;
    int cpu = processor_id();

    dmacfg = &(xlr_dmacfg[cpu][msg_type]);
    dmacfg->msg_type = msg_type;
    dmacfg->cache_w_coh = cache_w_coh;
    dmacfg->cache_r_coh = cache_r_coh;
    dmacfg->l2_alloc = l2_alloc;
    dmacfg->use_old_crc = use_old_crc;
    dmacfg->inform_src = inform_src;
    dmacfg->init_csum = init_csum;
    dmacfg->init_crc = init_crc;
    dmacfg->tid = 0;

    return dmacfg;
}


#endif // POST_DMA_H
