#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <headers/common.h>

// comvert string to uint64_t
uint64_t string2uint(const char *str)
{    
    // this number may have many formates
    // such as positive or negative, decimal or hexdecimal or octal
    // or their combination, even have spaces
    // e.g. 1234,-1234,0x1234,-0x1234,01234,-01234,  1234
    // e.g. we neglect octal and parse the type:01234 to decimal

    return string2uint_range(str, 0, -1);
}

uint64_t string2uint_range(const char *str, int start, int end)
{
    // -1 means the leaest element(length of str - 1)
    // the grammer like python
    end = (end == -1) ? strlen(str) - 1 : end;
    
    uint64_t uv = 0;  // unsigned value
    int sign_bit = 0; // 0 - positive; 1 - negative

    // DFA(确定有限状态机): deterministic finite automaton 
    /**/
    /**/
    /**/
    /**/
    int state = 0;
    // there are two ways to parse string
    // First is use if-else  (easy); Second if use dfa[][] (difficult)
    // we choice the easy way, use if-else
    for(int i = start; i <= end; i ++ )
    {
        char c = str[i];
        if(state == 0) // start
        {
            if(c == '0') 
            {
                state = 1; // hex or dec
                uv = 0;
                continue;
            }
            else if(c >= '1' && c <= '9') 
            {
                state = 2;  // dec
                uv = c - '0';
                continue;
            }
            else if(c == '-')
            {
                state = 3; // neg
                sign_bit = 1;
                continue;
            }
            else if(c == ' ')
            {
                state = 0;  // repeat start
                continue;
            }
            else { goto fail; }
        }
        else if(state == 1) // str[0]='0'
        {
            if(c >= '0' && c <= '9')
            {
                state = 2; // dec
                uv = uv * 10 + c - '0';
                continue; 
            }
            else if(c == 'x')
            {
                state = 4; // hex
                continue;
            }
            else if(c == ' ')
            {
                state = 6;  //  // meet space
                continue;
            }
            else { goto fail; }
        }
        else if(state == 2) // dec
        {
            if(c >= '0' && c <= '9')
            {
                state = 2;  // dec
                uint64_t pv = uv;   // pv: previous value
                uv = uv * 10 + c - '0';
                // maybe overflow
                // a wrong way to check(uv <= pv)
                // suck as 0x0 that (uv == pv) is valid
                if(uv < pv)    // truly overflow
                {
                    printf("(uint64_t)%s overflow: cannot convert\n", str);
                    goto fail;
                }
                continue;
            }
            else if(c == ' ')
            {
                state = 6;  //  // meet space
                continue;
            }
            else { goto fail; }
        }
        else if(state == 3) // negative
        {
            if(c == '0')
            {
                state = 1;  // negative_hex/dec
            }
            else if(c >= '1' && c <= '9')
            {
                state = 2; // negative_dec
                uv = c - '0';
                continue;
            }
            else { goto fail; }
        }
        else if(state == 4)
        {
            // First to add, so didn't to check overflow
            if(c >= '0' && c <= '9')
            {
                state = 5; // hex_cal
                uint64_t pv = uv;
                uv = uv * 16 + c - '0';
                // maybe overflow
                if(uv < pv)    // truly overflow
                {
                    printf("(uint64_t)%s overflow: cannot convert\n", str);
                    goto fail;
                }
                continue;
            }    
            else if(c >= 'a' &&  c <= 'f')
            {
                state = 5; // hex_cal
                uv = uv * 16 + (c - 'a' + 10);
                continue;
            }
            else { goto fail; }
        }
        else if(state == 5)
        {
            if(c >= '0' && c <= '9')
            {
                state = 5; // hex_cal
                uint64_t pv = uv;
                uv = uv * 16 + c - '0';
                // maybe overflow
                if(uv < pv)    // truly overflow
                {
                    printf("(uint64_t)%s overflow: cannot convert\n", str);
                    goto fail;
                }
                continue;
            }    
            else if(c >= 'a' &&  c <= 'f')
            {
                state = 5; // hex_cal
                uv = uv * 16 + (c - 'a' + 10);
                continue;
            }
            else { goto fail; }            
        }
        else if(state == 6)
        {
            if(c == ' ')
            {
                state = 6; // meet space
                continue;
            }
            else { goto fail; }
        }
    }

    if(sign_bit == 0)
    {
        return uv;
    }
    else if(sign_bit == 1)
    {
        if(uv >> 63 & 1) // 最高位为 1
        {
            printf("(int64_t)%s: signed overflow: canot convert!", str);
            exit(0);
        }    
        int64_t sv = -1 * (int64_t)uv;
        return *(uint64_t *)&sv; // return mapping(返回二进制布局)
    }

    fail: 
    printf("type converter: <%s> cannot be converted to integer\n", str);
    exit(0); // exit tht program to debug conveniently
}


// convert uint32_t to it's float
uint32_t uint2float(uint32_t u)
{
    if (u == 0x00000000)
    {
        return 0x00000000;
    }
    // must be NORMALIZED
    // counting the position of highest 1: u[n]
    int n = 31;
    while (0 <= n && (((u >> n) & 0x1) == 0x0))
    {
        n = n - 1;
    }
    uint32_t e, f;
    //    seee eeee efff ffff ffff ffff ffff ffff
    // <= 0000 0000 1111 1111 1111 1111 1111 1111
    if (u <= 0x00ffffff)
    {
        // no need rounding
        uint32_t mask = 0xffffffff >> (32 - n);
        f = (u & mask) << (23 - n);
        e = n + 127;
        return (e << 23) | f;
    }
    // >= 0000 0001 0000 0000 0000 0000 0000 0000
    else
    {
        // need rounding
        // expand to 64 bit for situations like 0xffffffff
        uint64_t a = 0;
        a += u;
        // compute g, r, s
        uint32_t g = (a >> (n - 23)) & 0x1;     // Guard bit, the lowest bit of the result
        uint32_t r = (a >> (n - 24)) & 0x1;     // Round bit, the highest bit to be removed
        uint32_t s = 0x0;                       // Sticky bit, the OR of remaining bits in the removed part (low)
        for (int j = 0; j < n - 24; ++ j)
        {
            s = s | ((u >> j) & 0x1);
        }
        // compute carry
        a = a >> (n - 23);
        // 0    1    ?    ... ?
        // [24] [23] [22] ... [0]
        /* Rounding Rules
            +-------+-------+-------+-------+
            |   G   |   R   |   S   |       |
            +-------+-------+-------+-------+
            |   0   |   0   |   0   |   +0  | round down
            |   0   |   0   |   1   |   +0  | round down
            |   0   |   1   |   0   |   +0  | round down
            |   0   |   1   |   1   |   +1  | round up
            |   1   |   0   |   0   |   +0  | round down
            |   1   |   0   |   1   |   +0  | round down
            |   1   |   1   |   0   |   +1  | round up
            |   1   |   1   |   1   |   +1  | round up
            +-------+-------+-------+-------+
        carry = R & (G | S) by K-Map
        */
        if ((r & (g | s)) == 0x1)
        {
            a = a + 1;
        }
        // check carry
        if ((a >> 23) == 0x1)
        {
            // 0    1    ?    ... ?
            // [24] [23] [22] ... [0]
            f = a & 0x007fffff;
            e = n + 127;
            return (e << 23) | f;
        }
        else if ((a >> 23) == 0x2)
        {
            // 1    0    0    ... 0
            // [24] [23] [22] ... [0]
            e = n + 1 + 127;
            return (e << 23);
        }
    }
    // inf as default error
    return 0x7f800000;
}
