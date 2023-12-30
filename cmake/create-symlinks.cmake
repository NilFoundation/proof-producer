message("Creating symlinks")

list(APPEND BINARIES proof-generator proof-generator-multithreaded)

file(MAKE_DIRECTORY $ENV{DESTDIR}/usr/bin)

foreach(BINARY IN LISTS BINARIES)
    set(LINK_NAME $ENV{DESTDIR}/usr/bin/${BINARY})
    set(LINK_TARGET "../lib/zkllvm/bin/${BINARY}")
    message("${LINK_NAME} -> ${LINK_TARGET}")
    file(CREATE_LINK ${LINK_TARGET} ${LINK_NAME} SYMBOLIC)
endforeach()