aux_source_directory(. DRCOMCLIENT_SRCS)
aux_source_directory(ui UI_SRCS)
aux_source_directory(network NETWORK_SRCS)

add_library(drcomclient ${DRCOMCLIENT_SRCS} ${UI_SRCS} ${NETWORK_SRCS})