#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct in6_addr {
    unsigned short Word[8];
} IN6_ADDR;

extern const IN6_ADDR in6addr_loopback = {
    0, 0, 0, 0, 0, 0, 0, 1,
};

#ifdef __cplusplus
}
#endif /* __cplusplus */
