EXECS=solver generator hash test

define FOREACH
    for DIR in $(EXECS); do \
        $(MAKE) -C src/$$DIR $(1); \
    done
endef

all: $(EXECS)

$(EXECS):
	$(MAKE) -C src/$@
	cp ./src/$@/$@ ./
clean:
	$(call FOREACH,clean)
	rm -f $(EXECS) 
# shell commands are a set of keystrokes away
distribute: clean
	tar zcvf dist.tgz *

# @ silences the printing of the command
# $(info ...) prints output
diff:
	$(info The status of the repository, and the volume of per-file changes:)
	@git status
	@git diff --stat

.PHONY: all $(EXECS)