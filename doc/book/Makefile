CSS = book.css
HTML_TEMPLATE = template.html
TEX_HEADER = header.tex
CHAPTERS = title.txt introduction.md environment_and_booting.md references.md
BIB = bibliography.bib
CITATION = citation_style.csl

all: book.html

book.html: $(CHAPTERS) $(CSS) $(HTML_TEMPLATE) $(BIB) $(CITATION)
	pandoc -s -S --toc -c $(CSS) --template $(HTML_TEMPLATE) \
		   --bibliography $(BIB) --csl $(CITATION) $(CHAPTERS) -o $@

book.pdf: $(CHAPTERS)
	markdown2pdf --toc -H $(TEX_HEADER) $(CHAPTERS) -o $@

clean:
	rm -f book.pdf book.html
