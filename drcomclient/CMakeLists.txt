aux_source_directory(. DIR_DRCOMCLIENT_SRCS)
aux_source_directory(ui DIR_UI_SRCS)
aux_source_directory(network DIR_NETWORK_SRCS)
aux_source_directory(util DIR_UTIL_SRCS)

add_library(drcomclient ${DIR_DRCOMCLIENT_SRCS} ${DIR_UI_SRCS} ${DIR_NETWORK_SRCS} ${DIR_UTIL_SRCS})