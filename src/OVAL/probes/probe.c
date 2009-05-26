#include <sexp-types.h>
#include <sexp-manip.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <xmalloc.h>
#include "oval_definitions_impl.h"
#include "oval_collection_impl.h"
#include "oval_agent_api_impl.h"
#include "probe.h"

#ifndef _A
#define _A(x) assert(x)
#endif

SEXP_t *SEXP_OVALattr_create (const char *name, ...)
{
        SEXP_t *list, *val;
        va_list ap;

        list = SEXP_list_new ();
        va_start(ap, name);
        
        while (name != NULL) {
                val = va_arg (ap, SEXP_t *);
                                
                if (val == NULL) {
                        SEXP_list_add (list, SEXP_string_new (name, strlen (name)));
                } else {
                        char  *attr_name;
                        size_t attr_namelen = strlen (name) + 1;
                        
                        SEXP_VALIDATE(val);

                        if (SEXP_listp (val)) {
                                _D("Invalid type of attribute value: list\n");
                                SEXP_free (list);
                                return (NULL);
                        }
                        
                        attr_name = xmalloc (sizeof (char) * (attr_namelen + 1));
                        snprintf (attr_name, attr_namelen + 1, ":%s", name);
                        
                        SEXP_list_add (list, SEXP_string_new (attr_name, attr_namelen));
                        SEXP_list_add (list, val);
                        
                        xfree ((void **)&attr_name);
                }
                
                name = va_arg (ap, const char *);
        }
        
        va_end (ap);

        return (list);
}

SEXP_t *SEXP_OVALelm_create (const char *name, ...)
{
        SEXP_t *list;
        SEXP_t *elm, *attrs, *value;
        va_list ap;
        
        list = SEXP_list_new ();
        va_start (ap, name);

        while (name != NULL) {
                attrs = va_arg (ap, SEXP_t *);
                value = va_arg (ap, SEXP_t *);

                elm = SEXP_list_new ();
                
                if (attrs != NULL) {
                        SEXP_t *elm_list;
                        
                        SEXP_VALIDATE(attrs);
                        elm_list = SEXP_list_new ();
                        SEXP_list_add (elm_list, SEXP_string_new (name, strlen (name)));
                        SEXP_list_join (elm_list, attrs);
                        SEXP_list_add (elm, elm_list);
                } else {
                        SEXP_list_add (elm, SEXP_string_new (name, strlen (name)));
                }
                
                if (value != NULL) {
                        SEXP_VALIDATE(value);
                        SEXP_list_add (elm, value);
                }
                
                SEXP_list_add (list, elm);
                
                name = va_arg (ap, const char *);
        }
        
        return (list);
}

/*
 * Examples of S-exp OVAL object (XML) representation:
 *
 * 1) object without attributes
 *    (rpminfo_state (name "foo") (arch "i386") ...)
 * 
 * 2) object with attributes
 *    ((rpminfo_state :attr1 "attrval") (name "foo") (arch "i386") ...)
 */

SEXP_t *SEXP_OVALobj_create (const char *obj_name, SEXP_t *obj_attrs, ...)
{
        SEXP_t *sexp = NULL, *sexp_tmp;
        uint16_t i;
        va_list ap;
        char *arg_name;
        SEXP_t *arg_value, *arg_attrs;
        
        _A(obj_name != NULL);
        
        sexp = SEXP_list_new ();
        
        if (obj_attrs == NULL) {
                SEXP_list_add (sexp, SEXP_string_new (obj_name, strlen (obj_name)));
        } else {
                SEXP_VALIDATE(obj_attrs);
                
                sexp_tmp = SEXP_list_new ();
                SEXP_list_add (sexp_tmp, SEXP_string_new (obj_name, strlen (obj_name)));
                sexp_tmp = SEXP_list_join (sexp_tmp, obj_attrs);                
                SEXP_list_add (sexp, sexp_tmp);
        }
        
        /* Add object elements */
        va_start (ap, obj_attrs);
        
        while ((arg_name  = va_arg (ap, char *)) != NULL) {
                arg_attrs = va_arg (ap, SEXP_t *);
                arg_value = va_arg (ap, SEXP_t *);
                
                sexp_tmp = SEXP_list_new ();
                
                if (arg_attrs == NULL) {
                        SEXP_list_add (sexp_tmp, SEXP_string_new (arg_name, strlen (arg_name)));
                } else {
                        /* add attrs */
                        SEXP_t *elm_list;
                        
                        elm_list = SEXP_list_new ();
                        SEXP_list_add (elm_list, SEXP_string_new (arg_name, strlen (arg_name)));
                        elm_list = SEXP_list_join (elm_list, arg_attrs);
                        SEXP_list_add (sexp_tmp, elm_list);
                }
                
                /* add value to the element list */
                SEXP_list_add (sexp_tmp, arg_value);
                
                /* add the element to the object */
                SEXP_list_add (sexp, sexp_tmp);
        }
        
        va_end (ap);
        
        return (sexp);
}

SEXP_t *SEXP_OVALobj_attr_add (SEXP_t *obj, const char *name, SEXP_t *value)
{
        SEXP_t *attr;

        _A(obj  != NULL);
        _A(name != NULL);

        SEXP_VALIDATE(obj);
        
        if (value != NULL) {
                SEXP_VALIDATE(value);
        }
        
        attr = SEXP_OVALattr_create (name, value);
        
        if (SEXP_listp (SEXP_list_first (obj))) {
                SEXP_list_join (SEXP_list_first (obj), attr);
        } else {
                SEXP_t *list;
                
                list = SEXP_list_new();
                SEXP_list_add (list, SEXP_list_first (obj));
                SEXP_list_add (list, attr);
                
                SEXP_copy (SEXP_list_first (obj), list);
        }
        
        return (obj);
}

SEXP_t *SEXP_OVALobj_attr_del (SEXP_t *obj, const char *name)
{
        /* TODO */
        return (NULL);
}

SEXP_t *SEXP_OVALobj_elm_add (SEXP_t *obj, const char *name, SEXP_t *attrs, SEXP_t *value)
{
        SEXP_t *elm;
        
        _A(name != NULL);
        _A(obj != NULL);

        SEXP_VALIDATE(obj);
        
        if (attrs != NULL) {
                SEXP_VALIDATE(attrs);
        }
                
        if (value != NULL) {
                SEXP_VALIDATE(value);
        }
        
        elm = SEXP_OVALelm_create (name, attrs, value);
        SEXP_list_add (obj, elm);
        
        return (obj);
}

SEXP_t *SEXP_OVALobj_elm_del (SEXP_t *obj, const char *name)
{
        /* TODO */
        return (NULL);
}

SEXP_t *SEXP_OVALobj_elmattr_add (SEXP_t *obj, const char *elm_name, const char *attr_name, SEXP_t *value)
{
        SEXP_t *elm;
        
        _A(obj != NULL);
        _A(elm_name != NULL);
        _A(attr_name != NULL);
        
        elm = SEXP_OVALobj_getelm (obj, elm_name);
        if (elm == NULL) {
                /* element not found */
                return (NULL);
        }
        
        return SEXP_OVALelm_attr_add (elm, attr_name, value);
}

SEXP_t *SEXP_OVALobj_elmattr_del (SEXP_t *obj, const char *elm_name, const char *attr_name)
{
        /* TODO */
        return (NULL);
}

SEXP_t *SEXP_OVALelm_attr_add (SEXP_t *elm, const char *name, SEXP_t *value)
{
        _A(elm != NULL);
        _A(name != NULL);
        
        SEXP_VALIDATE(elm);

        if (value != NULL) {
                SEXP_VALIDATE(value);
        }
        
        return SEXP_OVALobj_attr_add (elm, name, value);
}

SEXP_t *SEXP_OVALelm_attr_del (SEXP_t *elm, const char *name)
{
        /* TODO */
        return (NULL);
}

int SEXP_OVALobj_validate (SEXP_t *obj)
{
        /* TODO */
        return (1);
}

SEXP_t *SEXP_OVALobj_getelm (SEXP_t *obj, const char *name)
{
        uint32_t i;
        SEXP_t  *elm, *val, *elm_name;
        
        _A(name != NULL);
        SEXP_VALIDATE(obj);
        
        i = 2;
        while ((elm = SEXP_list_nth (obj, i)) != NULL) {
                elm_name = SEXP_list_first (elm);
                
                if (!SEXP_stringp (elm_name)) {
                        if (SEXP_listp (elm_name)) {
                                elm_name = SEXP_list_first (elm_name);
                        } else {
                                continue;
                        }
                }
                
                if (SEXP_strcmp (elm_name, name) == 0) {
                        return (elm);
                }
                
                ++i;
        }
        
        return (NULL);
}

SEXP_t *SEXP_OVALelm_getval (SEXP_t *elm)
{
        SEXP_t *val;

        SEXP_VALIDATE(elm);
 
        return (SEXP_list_nth (elm, 2));
}

SEXP_t *SEXP_OVALelm_getattrval (SEXP_t *elm, const char *name)
{
        SEXP_t *val, *attrs;

        SEXP_VALIDATE(elm);

        attrs = SEXP_list_first (elm);
        
        if (SEXP_listp (attrs)) {
                SEXP_t *attr;
                uint16_t i = 2;

                while ((attr = SEXP_list_nth (attrs, i)) != NULL) {
                        if (SEXP_stringp (attr)) {
                                char *attr_name;

                                attr_name = xmalloc (sizeof (char) * (strlen (name) + 2));
                                snprintf (attr_name, strlen (name) + 2, ":%s", name);

                                if (SEXP_strcmp (attr, attr_name) == 0) {
                                        attr = SEXP_list_nth (attrs, i + 1);
                                } else {
                                        xfree ((void **)&attr_name);
                                        ++i;
                                        continue;
                                }
                                
                                xfree ((void **)&attr_name);
                                return (attr);
                        }

                        ++i;
                }
        }
        
        return (NULL);
}

int SEXP_OVALelm_hasattr (SEXP_t *elm, const char *name)
{
        SEXP_t *attrs;
        int ret = 0;

        SEXP_VALIDATE(elm);

        attrs = SEXP_list_first (elm);
        
        if (SEXP_listp (attrs)) {
                SEXP_t *attr;
                uint16_t i = 2;
                
                while ((attr = SEXP_list_nth (attrs, i)) != NULL) {
                        if (SEXP_stringp (attr)) {
                                char *attr_name;
                                
                                attr_name = xmalloc (sizeof (char) * (strlen (name) + 2));
                                snprintf (attr_name, strlen (name) + 2, ":%s", name);
                                
                                if (SEXP_strcmp (attr, attr_name) == 0 ||
                                    SEXP_strcmp (attr, name) == 0) {
                                        ret = 1;
                                } else {
                                        xfree ((void **)&attr_name);
                                        ++i;
                                        continue;
                                }
                                
                                xfree ((void **)&attr_name);
                                return (ret);
                        }
                        
                        ++i;
                }
        }
        
        return (ret);
}

char *SEXP_OVALobj_getelm_val (SEXP_t *obj, const char *name)
{
        uint32_t i;
        SEXP_t  *elm, *val, *elm_name;

        _A(obj != NULL);
        _A(name != NULL);

        i = 2;
        while ((elm = SEXP_list_nth (obj, i)) != NULL) {
                elm_name = SEXP_list_first (elm);
                
                if (!SEXP_stringp (elm_name)) {
                        if (SEXP_listp (elm_name)) {
                                elm_name = SEXP_list_first (elm_name);
                        } else {
                                continue;
                        }
                }
                
                if (SEXP_strcmp (elm_name, name) == 0) {
                        val = SEXP_list_nth (elm, 2);
                        
                        if (SEXP_stringp (val))
                                return SEXP_string_cstr (val);
                        else 
                                return (NULL);
                }

                ++i;
        }
        
        return (NULL);
}

SEXP_t *oval_object_to_sexp (const char *typestr, struct oval_object *object)
{
        SEXP_t *sexp, *elm, *elm_value;
        struct oval_iterator_object_content *cit;
        struct oval_object_content *content;
        struct oval_entity *entity;
        char *obj_name;
        uint16_t obj_namelen;

        /* (typestr_object (ent1 val1) (ent2 val2) ...) */
        sexp = SEXP_list_new ();
        cit  = oval_object_object_content (object);
        
        /* give the object a name */
        obj_namelen = strlen (typestr) + strlen ("_object");
        obj_name    = xmalloc (sizeof (char) * (obj_namelen + 1));
        
        snprintf (obj_name, obj_namelen + 1, "%s_object", typestr);
        SEXP_list_add (sexp, SEXP_string_new (obj_name, obj_namelen));
        
        while (oval_iterator_object_content_has_more (cit)) {
                content   = oval_iterator_object_content_next (cit);
                elm_value = NULL;
                
                switch (oval_object_content_type (content)) {
                case OVAL_OBJECTCONTENT_ENTITY:
                {
                        char *elm_name;
                        
                        /* ENTITY BEG */
                        entity = oval_object_content_entity (content);
                        
                        switch (oval_entity_datatype (entity)) {
                        case OVAL_DATATYPE_VERSION:
                        case OVAL_DATATYPE_STRING:
                        case OVAL_DATATYPE_EVR_STRING:
                        {
                                char *strval = oval_value_text (oval_entity_value (entity));
                                elm_value    = SEXP_string_new (strval, strlen (strval));
                        } break;
                        case OVAL_DATATYPE_FLOAT:
                                /* TODO */
                                continue;
                        case OVAL_DATATYPE_INTEGER:
                                /* TODO */
                                continue;
                        case OVAL_DATATYPE_BOOLEAN:
                                /* TODO */
                                continue;
                        default:
                                continue;
                        }
                        
                        elm = SEXP_list_new ();
                        elm_name = oval_entity_name (entity);
                        
                        /* TODO: handle element attributes */
                        SEXP_list_add (elm, SEXP_string_new (elm_name, strlen (elm_name)));
                        if (elm_value != NULL)
                                SEXP_list_add (elm, elm_value);
                        
                        /* ENTITY END */
                } break;
                default:
                        continue;
                }
                
                /* add the prepared element */
                SEXP_list_add (sexp, elm);
        }
        
        return (sexp);
}
