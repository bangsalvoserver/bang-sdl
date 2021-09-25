#ifndef __RESOURCE_H__
#define __RESOURCE_H__

struct resource {
    const char *data;
    int length;
};

#define RESOURCE_NAME(name) __resource__##name
#define RESOURCE_LENGTH(name) __resource__##name##_length
#define DECLARE_RESOURCE(name) extern const char RESOURCE_NAME(name)[]; extern const int RESOURCE_LENGTH(name);
#define GET_RESOURCE(name) resource{RESOURCE_NAME(name), RESOURCE_LENGTH(name)}

#endif