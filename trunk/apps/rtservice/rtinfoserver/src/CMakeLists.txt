SET(SRC bdb_svc.c
	store.c
	main.c
	)

ADD_DEFINITIONS(-D_LARGEFILE64_SOURCE)

SET(APR_DIR /usr/local/apr)
SET(APR_UTIL_DIR /usr/local/apr)
SET(BERKELEY_DB_DIR /home/zhoubug/dev/BerkeleyDB.4.5)
SET(ZEVENT_SOURCE_DIR /home/zhoubug/dev/svn_work/zsh/zevent)
SET(RTINFOSERVER_SOURCE_DIR /home/zhoubug/dev/svn_work/zsh/zevent/apps/rtservice/rtinfoserver)

SET(INC_DIRS ${APR_DIR}/include/apr-1 ${APR_UTIL_DIR}/include/apr-1 ${BERKELEY_DB_DIR}/include 
	${ZEVENT_SOURCE_DIR}/include ${RTINFOSERVER_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${INC_DIRS})

SET(LINK_DIRS ${APR_DIR}/lib ${APR_UTIL_DIR}/lib ${BERKELEY_DB_DIR}/lib ${ZEVENT_SOURCE_DIR}/usr/bin)
LINK_DIRECTORIES(${LINK_DIRS})

SET(ZEVENT_LIB zevent)
SET(APR_LIB apr-1)
SET(APR_UTIL_LIB aprutil-1)
SET(BERKELEY_DB_LIB db)

ADD_EXECUTABLE(main ${SRC})

TARGET_LINK_LIBRARIES(main ${APR_LIB} ${APR_UTIL_LIB} ${BERKELEY_DB_LIB} ${ZEVENT_LIB})

#INSTALL(TARGETS zevent main RUNTIME DESTINATION bin
#	LIBRARY DESTINATION bin)

