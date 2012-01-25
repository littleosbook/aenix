CSS  = pandoc.css
HTML_TEMPLATE = template.html
TEX_HEADER = header.tex
DOCS = paging.md
HTML = $(DOCS:.md=.html)
PDF  = $(DOCS:.md=.pdf)

all: html

html: $(HTML)

pdf: $(PDF)

%.html: %.md
	pandoc -s -S --toc -c $(CSS) --template $(HTML_TEMPLATE) $< -o $@

%.pdf: %.md
	markdown2pdf --toc -H $(TEX_HEADER) $< -o $@

clean:
	rm -f $(HTML) $(PDF)
