# My Additions:
build_example(
  NAME active_passive
  SOURCE_FILES
  	active_passive.cc
	overlayApplication.h
	overlayApplication.cc
	hostApp.h
	hostApp.cc
	ueApp.h
	ueApp.cc
	SDtag.h
	SDtag.cc
	utils.h
	utils.cc
	netmeta.h
	netmeta.cc
  LIBRARIES_TO_LINK
    ${libnetanim}
  	${libcore}
  	${libapplications}
  	${libpoint-to-point}
  	${libpoint-to-point-layout}
  	${libinternet}
  	${libcsma}
	${libflow-monitor}
)

# Libraries to link:
#LIBRARIES_TO_LINK(
#  ${libnetanim}
#  ${libcore}
#  ${libapplications}
#  ${libpoint-to-point}
#  ${libpoint-to-point-layout}
#  ${libinternet}
#  ${libcsma}
#)
