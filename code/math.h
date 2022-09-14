#if !defined(MATH_H)
#define MATH_H
// Eventually get rid of this file and everything in it


static v2
calc_center(v2 *p, u32 count){
    v2 result = {0};

    for(u32 i=0; i<count; ++i){
        result.x += p->x;
        result.y += p->y;
        p++;
    }
    p -= count;

    result.x /= count;
    result.y /= count;

    return(result);
}

static void
translate(v2 *p, u32 count, v2 translation){
    for(u32 i=0; i<count; ++i){
        *p = *p + translation;
        p++;
    }
    p -= count;
}

static void
scale(v2 *p, u32 count, f32 scalar, v2 origin){
    for(u32 i=0; i<count; ++i){
        *p = ((*p - origin) * scalar) + origin;
        p++;
    }
    p -= count;
}

static v2
rotate_point(v2 p, f32 angle, v2 origin){
    v2 result = {0};
    result.x = (p.x - origin.x) * cos(angle * 3.14f/180.0f) - (p.y - origin.y) * sin(angle * 3.14f/180.0f) + origin.x;
    result.y = (p.x - origin.x) * sin(angle * 3.14f/180.0f) + (p.y - origin.y) * cos(angle * 3.14f/180.0f) + origin.y;
    return(result);
}

static void
rotate_points(v2 *p, u32 count, f32 angle, v2 origin){
    for(u32 i=0; i<count; ++i){
        v2 result = {0};
        result = rotate_point(*p, angle, origin);
        p->x = result.x;
        p->y = result.y;
        p++;
    }
    p -= count;
}

static void
swap_v2(v2 *a, v2 *b){
    v2 t = *a;
    *a = *b;
    *b = t;
}


// INCOMPLETE: LOOK AT SIZE_T VS INT DIFFERENCES
static void
scale_pts(v2 *p, size_t count, f32 s){
    for(s32 i=0; i < (int)count; ++i){
        *p = (*p * s);
        p++;
    }
}

#endif
