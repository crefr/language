BUILD = LINUX

FRONTEND_DIR  = frontend/
BACKEND_DIR   = backend/
MIDDLEEND_DIR = middleend/

.PHONY: frontend
.PHONY: backend
.PHONY: middleend

all: frontend backend


backend:
	cd $(BACKEND_DIR)  && make BUILD=$(BUILD)

frontend:
	cd $(FRONTEND_DIR) && make BUILD=$(BUILD)

middleend:
	cd $(MIDDLEEND_DIR) && make BUILD=$(BUILD)

clean:
	cd $(FRONTEND_DIR) && make clean
	cd $(BACKEND_DIR)  && make clean
