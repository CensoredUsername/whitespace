#ifndef WSINT_H
#define WSINT_H

/* inspiration from:
 * http://svn.python.org/projects/python/trunk/Objects/longobject.c
 * https://stackoverflow.com/questions/1218149/arbitrary-precision-arithmetic-explanation
 * http://homepage.cs.uiowa.edu/~jones/bcd/decimal.html
 * http://www.cs.sunysb.edu/~skiena/392/programs/bignum.c
 * 
 */

#define ACTLEN(x) ((x) & ~(1U<<31))
#define SIGN(x) (((x) & (1U<<31))? -1: 1)

typedef int32_t sdigit;
typedef int64_t stwodigits;
typedef uint32_t digit;
typedef uint64_t twodigits;
#define WS_INT_SHIFT 30
#define WS_INT_BASE ((digit)1<<WS_INT_SHIFT)
#define WS_INT_MASK ((digit)(WS_INT_BASE-1))
#define WS_INT_SIGN_SHIFT 31
#define WS_INT_SIGN_MASK (1U<<WS_INT_SIGN_SHIFT)

#define WS_INT_DEC_BASE ((digit)1000000000)
#define WS_INT_DEC_SHIFT 9 //10**9

/* the format of the datastructure used is defined accordingly
 * if length == 0, then data is used, and we just represent the data as an int. this is nice and fast
 * otherwise, we store parts of the number in an array of unsigned ints. the sign bit of length is abused as
 * the sign of the actual number, and the rest of the int is a measure for how long the int is
 */
typedef struct {
    digit length;
    union {
        sdigit data;
        digit *digits;
    };
} ws_int;
/* Note: If -2^30 >= value or value >= 2^30 (WS_INT_BASE), it should always be a big int. this is to prevent weird cases
 * in I/O functions. If it's between those values, it can be either an int or a long int.
 */ 

void ws_int_normalize(ws_int *const input) {
    //nuke trailing zeros.
    if (!input->length) {
        return;
    }
    size_t length = ACTLEN(input->length);
    size_t i = length;
    while (i > 1 && !input->digits[i-1]) {
        i--;
    }
    /*if (i >= 1) { // currently commented out because I doubt this would be used a lot and it's not worth fixing
        sdigit value = SIGN(input->length) * (sdigit)input->digits[0];
        free(input->digits);
        input->data = value;
        input->length = 0;
    }*/
    if (i != length) {
        input->digits = realloc(input->digits, sizeof(digit) * i);
        input->length = i | (input->length & WS_INT_SIGN_MASK);
    }
}

void ws_int_free(const ws_int *const input) {
    
    //if it's a long int, free the digits
    if (input->length) {
        free(input->digits);
    }
}

void ws_int_copy(ws_int *const result, const ws_int *const input) {

    //copy the result, and allocate a new array for the digits if necessary
    if (!input->length) {
        *result = *input;

    } else {
        size_t length = (size_t)ACTLEN(input->length);
        result->length = input->length;
        result->digits = (digit *)malloc(sizeof(digit) * length);
        for (size_t i = 0; i < length; i++) {
            result->digits[i] = input->digits[i];
        }
    }
}

void ws_int_from_int(ws_int *const result, sdigit input, digit *const digitbuffer) {

    // construct an ws_int out of an int (actually an sdigit which is a int32_t). 
    // if digitbuffer is given, it should be an array of length 2 or larger. It will force the number to be a long int
    if (input >= (sdigit)WS_INT_BASE || (-input) >= (sdigit)WS_INT_BASE || digitbuffer) {

        // calculate the required length in a simplified manner
        result->length = 0;
        if (input < 0) {
            input = -input;
            result->length |= WS_INT_SIGN_MASK;
        }
        digit reqlen = (input >> WS_INT_SHIFT)? 2: 1;
        result->length |= reqlen;

        if (!digitbuffer) {
            result->digits = (digit *)malloc(sizeof(digit) * reqlen);
        } else {
            result->digits = digitbuffer;
        }

        // and a simplified calculation of the two digits.
        result->digits[0] = input & WS_INT_MASK;
        if (reqlen == 2) {
            result->digits[1] = input >> WS_INT_SHIFT;
        }

    } else {

        // or if it just fits within BASE
        result->length = 0;
        result->data = input;
    }
}

void ws_int_from_long(ws_int *const result, stwodigits input, digit *const digitbuffer) {

    // construct an ws_int out of a long (actually an stwodigits which is a int64_t). 
    // if digitbuffer is given, it should be an array of length 3 or larger. It will force the number to be a long int

    if (input >= (sdigit)WS_INT_BASE || -input >= (sdigit)WS_INT_BASE || digitbuffer) {

        // calculate the required length in a simplified manner
        result->length = 0;
        if (input < 0) {
            input = -input;
            result->length |= WS_INT_SIGN_MASK;
        }
        digit reqlen = (input >> (2*WS_INT_SHIFT))? 3: (input >> WS_INT_SHIFT)? 2: 1;
        result->length |= reqlen;

        if (!digitbuffer) {
            result->digits = (digit *)malloc(sizeof(digit) * reqlen);
        } else {
            result->digits = digitbuffer;
        }

        // and a simplified calculation of the three digits.
        result->digits[0] = input & WS_INT_MASK;
        if (reqlen >= 2) {
            result->digits[1] = (input >> WS_INT_SHIFT) & WS_INT_MASK;
        }
        if (reqlen == 3) {
            result->digits[2] = (input >> (2*WS_INT_SHIFT));
        }
        ws_int_normalize(result);

    } else {

        // or if it just fits within BASE
        result->length = 0;
        result->data = (sdigit)input;
    }
}

void ws_int_from_whitespace(ws_int *const result, const ws_string *const string) {

    if(string->length < 2) {
        //protect against empty parameters or sign only parameters
        result->length = 0;
        result->data = 0;

    } else if (string->length <= WS_INT_SHIFT+1) { //since the sign is included in the length it's <= instead of <

        // with this size we can be sure that the parameter is just a normal int and not a bigint
        sdigit accumulator = 0;

        for(size_t i = string->length-1; i > 0; i--) {
            if (string->data[0] == SPACE) {
                accumulator += (string->data[i] == TAB) << (string->length - 1 - i);
            } else {
                accumulator -= (string->data[i] == TAB) << (string->length - 1 - i);
            }
        }
        result->length = 0;
        result->data = accumulator;

    } else {

        //a long int. since we come from a binary base this is relatively simple though
        result->length = (string->length-1) / WS_INT_SHIFT + !!((string->length-1) % WS_INT_SHIFT);
        result->digits = (digit *)malloc(sizeof(digit) * result->length);

        digit accumulator = 0;
        size_t accum_position = 0;
        digit *current_digit = result->digits;

        for (size_t i = string->length - 1; i > 0; i--) { //ignore 0, SIGN BIT
            accumulator |= (digit)((string->data[i] == TAB)? 1: 0) << accum_position++;

            //we're done with this digit;
            if (accum_position >= WS_INT_SHIFT) {
                *current_digit++ = accumulator;
                accumulator = 0;
                accum_position = 0;
            } 
        }

        // finish up uppermost unfilled digit
        if (accum_position) {
            *current_digit++ = accumulator;
        }

        // check sign
        if (string->data[0] == TAB) {
            result->length |= 0x80000000U;
        }
        ws_int_normalize(result);
    }
}

sdigit ws_int_to_int(const ws_int *const input) {

    //try to print a ws_int as an int. If the value is out of base bounds though, 
    //simply return 0x7FFFFFFF (sint32_max)
    if (input->length) {

        //check if the int is within base length. aka check if all digits above 0 are 0
        for (size_t i = 1; i < ACTLEN(input->length); i++) if (input->digits[i]) {
            return 0x7FFFFFFF; //can't convert to int with certainty
        }

        if (SIGN(input->length) == 1) {
            return (sdigit)input->digits[0];
        } else {
            return -(sdigit)input->digits[0];
        }
        
    } else {
        return input->data;
    }
}

char *ws_int_to_dec_string(const ws_int *const input) {
    char *buffer;
    if (!input->length) {
        buffer = malloc(12); //normal int is up to 10 decimal characters, 1 "-", 1 NUL byte
        sprintf(buffer, "%d", input->data);

    } else {
        size_t length = ACTLEN(input->length);
        
        size_t decarraysize = 1 + length * WS_INT_SHIFT / (3 * WS_INT_DEC_SHIFT);
        digit *decarray = (digit *)malloc(sizeof(digit) * decarraysize);

        // convert base 2**30 repr into base 10**9 repr
        digit *pin = input->digits;
        digit *pout = decarray;
        size_t size = 0;
        for (int i = length; --i >= 0;) {
            digit hi = pin[i];
            for (int j = 0; j < size; j++) {
                twodigits interim = (twodigits)pout[j] << WS_INT_SHIFT | hi;
                hi = (digit)(interim / WS_INT_DEC_BASE);
                pout[j] = (digit)(interim - (twodigits)hi * WS_INT_DEC_BASE);
            }
            while(hi) {
                pout[size++] = hi % WS_INT_DEC_BASE;
                hi /= WS_INT_DEC_BASE;
            }
        }

        if (size == 0) {
            pout[size++] = 0;
        }

        // calculate required length for buffer
        size_t bufflen = (SIGN(input->length) < 0) + 1 + (size - 1) * WS_INT_DEC_SHIFT; //+1;
        digit tenpow = 10;
        digit rem = pout[size-1];
        while (rem >= tenpow) {
            tenpow *= 10;
            bufflen++;
        }

        buffer = (char *)malloc(bufflen + 1);
        char *buffpos = buffer + bufflen;

        // write the base 10**9 repr to a string
        *buffpos = '\0';
        //*--buffpos = 'L';
        int i;
        for (i = 0; i < size - 1; i++) {
            rem = pout[i];
            for (int j = 0; j < WS_INT_DEC_SHIFT; j++) {
                *--buffpos = '0' + rem % 10;
                rem /= 10;
            }
        }

        rem = pout[i];
        do {
            *--buffpos = '0' + rem % 10;
            rem /= 10;
        } while (rem != 0);
        
        if (SIGN(input->length) < 0) {
            *--buffpos = '-';
        }

        //clean up
        free(decarray);
    }
    return buffer;
}

void ws_int_print(const ws_int *const input) {
    char *str = ws_int_to_dec_string(input);
    printf("%s", str);
    free(str);
}

void ws_int_input(ws_int *const result) { 
    // not ready for bigints yet
    int input;
    scanf("%d", &input);
    ws_int_from_int(result, input, NULL);
}

void ws_int_multiply(ws_int *const result, const ws_int *left, const ws_int *right) {
    if (!left->length && !right->length) {

        //calculate in a format which can hold a sdigit overflow
        stwodigits interim = ((stwodigits)left->data) * ((stwodigits)right->data);

        ws_int_from_long(result, interim, NULL);

    } else {

        //upgrade if one of them isn't a big int
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;
        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;
        }

        // bigint algorithm
        size_t leftlength = ACTLEN(left->length);
        size_t rightlength = ACTLEN(right->length);

        result->length = leftlength + rightlength;
        result->digits = (digit *)malloc(sizeof(digit) * result->length);
        memset(result->digits, 0, sizeof(digit) * result->length);

        //actual multiplication algorithm
        twodigits carry;
        twodigits current;
        digit *resp;
        digit *rightp;
        const digit *rightend;

        for (size_t i = 0; i < leftlength; i++) {
            carry = 0;
            current = left->digits[i];
            resp = result->digits + i;
            rightp = right->digits;
            rightend = right->digits + rightlength;

            while (rightp < rightend) {
                carry += *resp + *rightp++ * current;
                *resp++ = (digit)(carry & WS_INT_MASK);
                carry >>= WS_INT_SHIFT; 
            }
            if (carry) {
                *resp += (digit)(carry & WS_INT_MASK);
            }
        }

        //fix sign and normalize
        result->length |= WS_INT_SIGN_MASK & (left->length ^ right->length);
        ws_int_normalize(result);
    }
}

void ws_int_divide(ws_int *const result, const ws_int *left, const ws_int *right) {
    // currently no bigint support yet
    if (!left->length && !right->length) {

        //for int divide, a / b = c, c < a so don't have to worry about overflows here
        sdigit interim = left->data / right->data;
        
        //updrade to bigint if necessary
        ws_int_from_int(result, interim, NULL);

    } else {

        //upgrade if one of them isn't a big int
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;
        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;
        }

        // bigint algorithm
        printf("not implemented yet");
        exit(EXIT_FAILURE);
    }
}

void ws_int_modulo(ws_int *const result, const ws_int *left, const ws_int *right) {
    // currently no bigint support yet
    if (!left->length && !right->length) {

        //for int modulo, a % b = c, c <= a, don't have to worry about overflows here
        sdigit interim = left->data % right->data;
        
        //updrade to bigint if necessary
        ws_int_from_int(result, interim, NULL);
        
    } else {

        //upgrade if one of them isn't a big int
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;
        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;
        }

        // bigint algorithm
        printf("not implemented yet");
        exit(EXIT_FAILURE);
    }
}

// the ws_long_sub and ws_long_add functions ignore the sign of the left and right inputs.
// this is where the actual implementation of long addition and subtraction resides
static void ws_long_add(ws_int *const result, const ws_int *left, const ws_int *right) {
    size_t leftlength = ACTLEN(left->length);
    size_t rightlength = ACTLEN(right->length);

    //ensure that left is the largest one for convenience
    if (leftlength < rightlength) {
        const ws_int *temp = left;
        left = right;
        right = temp;

        size_t templength = leftlength;
        leftlength = rightlength;
        rightlength = templength;
    }

    result->length = leftlength + 1;
    result->digits = (digit *)malloc(sizeof(digit) * result->length);
    digit carry = 0;
    size_t i = 0;

    for (; i < rightlength; i++) {
        carry += left->digits[i] + right->digits[i];
        result->digits[i] = carry & WS_INT_MASK;
        carry >>= WS_INT_SHIFT;
    }
    for (; i < leftlength; i++) {
        carry += left->digits[i];
        result->digits[i] = carry & WS_INT_MASK;
        carry >>= WS_INT_SHIFT;
    }
    result->digits[i] = carry;
}

static void ws_long_sub(ws_int *const result, const ws_int *left, const ws_int *right) {
    size_t leftlength = ACTLEN(left->length);
    size_t rightlength = ACTLEN(right->length);
    int sign = 0;
    digit borrow = 0;

    //ensure that left is the largest one to guarantee no sign changes
    size_t i = (leftlength > rightlength)? leftlength: rightlength;
    for (i-- ; i >= 0; i--) {
        if (i >= rightlength && left->digits[i]) {
            sign = 1;
            break;
        } else if (i >= leftlength && right->digits[i]) {
            sign = -1;
            break;
        } else if (i < leftlength && i < rightlength) {
            if (left->digits[i] > right->digits[i]) {
                sign = 1;
                break;
            } else if (left->digits[i] < right->digits[i]) {
                sign = -1;
                break;
            }
        }
    }
    if (sign == 0) {
        //if they're equal we're not doing difficult things
        result->length = 1;
        result->digits = (digit *)malloc(sizeof(digit) * 1);
        result->digits[0] = 0;
        return;

    } else if (sign < 0) {
        //make sure left is the largest so the answer is always positive
        const ws_int *temp = left;
        left = right;
        right = temp;

        size_t templength = leftlength;
        leftlength = rightlength;
        rightlength = templength;
    }

    //abusing leftlength and rightlength to hold data
    result->length = i + 1; //the first index at which left and right digits have different values
    result->digits = (digit *)malloc(sizeof(digit) * result->length);

    leftlength = (result->length > leftlength)? leftlength: result->length; // MIN
    rightlength = (result->length > rightlength)? rightlength: result->length; // MIN
    // note: leftlength >= rightlength per definition

    size_t j = 0;
    for (; j < rightlength; j++) {
        borrow = left->digits[j] - right->digits[j] - borrow;
        result->digits[j] = borrow & WS_INT_MASK;
        borrow >>= WS_INT_SHIFT;
        borrow &= 1;
    }
    for (; j < leftlength; j++) {
        borrow = left->digits[j] - borrow;
        result->digits[j] = borrow & WS_INT_MASK;
        borrow >>= WS_INT_SHIFT;
        borrow &= 1;
    }
    if (sign < 0) {
        result->length |= WS_INT_SIGN_MASK;
    }
}

void ws_int_add(ws_int *const result, const ws_int *left, const ws_int *right) {
    if (!left->length && !right->length) {

        //calculate in a format which can hold an overflow
        sdigit interim = left->data + right->data;

        //updrade to bigint if necessary
        ws_int_from_int(result, interim, NULL);
        
    } else {

        //upgrade if one of them isn't a big int
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;
        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;
        }

        // bigint algorithm
        if (SIGN(left->length) > 0) {
            if (SIGN(right->length) > 0) {
                ws_long_add(result, left, right);
            } else {
                ws_long_sub(result, left, right);
            }
        } else {
            if (SIGN(right->length) > 0) {
                ws_long_sub(result, right, left);
            } else {
                ws_long_add(result, left, right);
                result->length ^= WS_INT_SIGN_MASK;
            }
        }
        ws_int_normalize(result);
    }
}

void ws_int_subtract(ws_int *const result, const ws_int *left, const ws_int *right) {
    if (!left->length && !right->length) {

        // calculate in a format which can hold an overflow
        sdigit interim = left->data - right->data;

        //updrade to bigint
        ws_int_from_int(result, interim, NULL);
        
    } else {

        //upgrade if one of them isn't a big int
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;
        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;
        }

        // bigint algorithm
        if (SIGN(left->length) > 0) {
            if (SIGN(right->length) > 0) {
                ws_long_sub(result, left, right);
            } else {
                ws_long_add(result, left, right);
            }
        } else {
            if (SIGN(right->length) > 0) {
                ws_long_add(result, right, left);
            } else {
                ws_long_sub(result, left, right);
            }
            result->length ^= WS_INT_SIGN_MASK;
        }
        ws_int_normalize(result);
    }
}

unsigned int ws_int_hash(const ws_int *const input) {
    if (!input->length) {
        return (unsigned int)input->data;

    } else {
        unsigned int accumulator = input->length;
        for (size_t i = 0; i < ACTLEN(input->length); i++) {
            accumulator ^= input->digits[i];
        }
        return accumulator;
    }
}

int ws_int_compare(const ws_int *left, const ws_int *right) {
    // note: just like the other cmp functions in the stdlib, this returns 0 on succes

    // it is not sure if a long is in the int range, so we have to check for mixed case the hard way
    if (!(left->length || right->length)) {
        return left->data != right->data;

    } else  {

        // make sure they're both long ints
        digit tempdigits[2];
        ws_int temp;
        if (!left->length) {
            ws_int_from_int(&temp, left->data, tempdigits);
            left = &temp;

        } else if (!right->length) {
            ws_int_from_int(&temp, right->data, tempdigits);
            right = &temp;

        }

        if (SIGN(left->length) != SIGN(right->length)) {

            //even with different signs, they can be equal if they're 0 due to one's complement being used
            for (size_t i = 0; i < ACTLEN(left->length); i++) if (left->digits[i]) {
                return 1;
            }
            for (size_t i = 0; i < ACTLEN(right->length); i++) if (right->digits[i]) {
                return 1;
            }

            return 0;

        } else {
            size_t i = 0;
            for (; i < ACTLEN(left->length) && i < ACTLEN(right->length); i++) if (right->digits[i] != left->digits[i]) {
                return 1;
            }

            //only one of the following loops will actually be evaluated of course
            for (;i < ACTLEN(left->length); i++) if (left->digits[i]) {
                return 1;
            }

            for (;i < ACTLEN(right->length); i++) if (right->digits[i]) {
                return 1;
            }

            return 0;
        }
    }
}

int ws_int_iszero(const ws_int *const input) {
    if (!input->length) {
        return input->data == 0;

    } else {

        //check if we are not 0
        for (size_t i = 0; i < ACTLEN(input->length); i++) if (input->digits[i]) {
            return 0;
        }

        return 1;
    }
}

int ws_int_isnegative(const ws_int *const input) {
    if (!input->length) {
        return input->data < 0;

    } else {
        // check if the sign is not positive
        if (SIGN(input->length) == 1) {
            return 0;
        }

        // If we're not 0 this is negative
        for (size_t i = 0; i < ACTLEN(input->length); i++) if (input->digits[i]) {
            return 1;
        }

        return 0;
    }
}

#endif