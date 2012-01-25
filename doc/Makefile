CSS  = pandoc.css
DOCS = paging.html

all: $(DOCS)

%.html: %.md
	pandoc -s -S --toc -c $(CSS) $< -o $@

clean:
	rm -f $(DOCS)
