
#INCLUDE(UseLATEX.cmake)
#include(FindLATEX.cmake)
find_package(LATEX COMPONENTS PDFLATEX)


#include(UseLATEX)

#ADD_LATEX_DOCUMENT(Doc.tex
#    #BIBFILES mybib.bib
#    IMAGE_DIRS images
#    DEFAULT_PDF
#    )
