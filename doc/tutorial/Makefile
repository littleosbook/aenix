CSS = book.css
HTML_TEMPLATE = template.html
TEX_HEADER = header.tex
SECTIONS = title.txt introduction.md environment_and_booting.md

all: book.html

book.html: $(SECTIONS) $(CSS) $(HTML_TEMPLATE)
	pandoc -s -S --toc -c $(CSS) --template $(HTML_TEMPLATE) $(SECTIONS) -o $@

book.pdf: $(SECTIONS)
	markdown2pdf --toc -H $(TEX_HEADER) $(SECTIONS) -o $@

clean:
	rm -f book.pdf book.html
