#include <stdio.h>
#include <stdlib.h>
#include "attrib.h"

int
main() {
	struct attrib_e * e = attrib_enew();
	const char * expression[] = {
		"-(-R1*(2+R3/(1.2-0.3)))",
		"R3^",
		"R3+R0",
	};
	int i;
	for (i = 0; i < sizeof(expression) / sizeof(expression[0]); ++i) {
		const char * err = attrib_epush(e, i, expression[i]);
		if (err) {
			printf("%s : %s\n", expression[i], err);
			exit(1);
		}
	}
	const char * err = attrib_einit(e);
	if (err) {
		printf("%s\n", err);
		exit(1);
	}
	
	printf("----------------\n");
	attrib_edump(e);
	printf("----------------\n");

	struct attrib * a = attrib_new();
	attrib_attach(a, e);
	attrib_write(a, 3, 8.0f);
	for (i = 0; i <= 3; ++i) {
		printf("R%d = %g\n", i, attrib_read(a, i));
	}

	attrib_delete(a);
	attrib_edelete(e);
	return 0;
}