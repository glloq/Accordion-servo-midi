# Tests natifs : se compilent avec un simple g++, sans toolchain AVR ni acces reseau.
#
#   make        -> compile et execute toute la suite
#   make clean
#
# Les bibliotheques Arduino sont remplacees par les stubs de test/stubs/.

CXX      ?= g++
CXXFLAGS ?= -std=gnu++11 -O1 -Wall -Wextra -Wno-unused-parameter
SRC_DIR   = ../accordionV06
BUILD_DIR = build

INCLUDES = -I. -Istubs -I$(SRC_DIR)

FIRMWARE_SRC = \
	$(SRC_DIR)/servoController.cpp \
	$(SRC_DIR)/handController.cpp \
	$(SRC_DIR)/bellowController.cpp \
	$(SRC_DIR)/instrument.cpp

BINARIES = $(BUILD_DIR)/test_note_mapping $(BUILD_DIR)/test_behavior

.PHONY: all test clean
all: test

test: $(BINARIES)
	@echo
	@for t in $(BINARIES); do \
		echo "### $$t"; \
		$$t || exit 1; \
	done
	@echo "Tous les tests sont passes."

# test_note_mapping ne depend que de noteMapping.h (aucun stub necessaire)
$(BUILD_DIR)/test_note_mapping: test_note_mapping.cpp $(SRC_DIR)/noteMapping.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I. -I$(SRC_DIR) $< -o $@

$(BUILD_DIR)/test_behavior: test_behavior.cpp stubs/stubs.cpp $(FIRMWARE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
