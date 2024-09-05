
cube.exe: cube.c cube-frag.spv cube-vert.spv
	gcc -std=c99 $< -lvulkan-1 -lgdi32 -Llib -Iinclude -o $@

cube-vert.spv: cube.vert
	glslc $< -o $@

cube-frag.spv: cube.frag
	glslc $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	del cube.exe 
	del cube-frag.spv
	del cube-vert.spv

run: cube.exe
	.\cube.exe

.PHONY: run