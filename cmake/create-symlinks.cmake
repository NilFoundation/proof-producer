message("Creating symlinks")

list(APPEND BINARIES proof-generator-single-threaded proof-generator-multi-threaded proof2json)

file(MAKE_DIRECTORY $ENV{DESTDIR}/usr/bin)

foreach(BINARY IN LISTS BINARIES)
    set(LINK_NAME $ENV{DESTDIR}/usr/bin/${BINARY})
    set(LINK_TARGET "${CMAKE_INSTALL_PREFIX}/bin/${BINARY}")
    message("${LINK_NAME} -> ${LINK_TARGET}")
    file(CREATE_LINK ${LINK_TARGET} ${LINK_NAME} SYMBOLIC)
endforeach()