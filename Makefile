BUILD = LINUX

FRONTEND_DIR = frontend/
BACKEND_DIR  = backend/

.PHONY: frontend
.PHONY: backend

all: frontend backend


backend:
	cd $(BACKEND_DIR)  && make BUILD=$(BUILD)

frontend:
	cd $(FRONTEND_DIR) && make BUILD=$(BUILD)

clean:
	cd $(FRONTEND_DIR) && make clean
	cd $(BACKEND_DIR)  && make clean
