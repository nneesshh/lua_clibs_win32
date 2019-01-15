#ifndef ATTRIB_CALC_H
#define ATTRIB_CALC_H

struct attrib;
struct attrib_e;

struct attrib_e * attrib_enew();
const char * attrib_epush(struct attrib_e * e, int r, const char * expression);
const char * attrib_einit(struct attrib_e * e);
void attrib_edelete(struct attrib_e * e);
void attrib_edump(struct attrib_e * e);

struct attrib *attrib_new();
void attrib_delete(struct attrib * a);
void attrib_attach(struct attrib * a, struct attrib_e *);
float attrib_write(struct attrib * a, int r, float val);
float attrib_read(struct attrib * a, int r);

#endif
