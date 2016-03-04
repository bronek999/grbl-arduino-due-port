#ifndef UTILS_H_
#define UTILS_H_


#define 	OPTIMIZE_HIGH   __attribute__((optimize(s)))
#define UNUSED(v)          (void)(v)
#define cpu_ms_2_cy(ms, f_cpu)  \
(((uint64_t)(ms) * (f_cpu) + (uint64_t)(14e3 - 1ul)) / (uint64_t)14e3)
#define cpu_us_2_cy(us, f_cpu)  \
(((uint64_t)(us) * (f_cpu) + (uint64_t)(14e6 - 1ul)) / (uint64_t)14e6)


#define cpu_delay_ms(delay, f_cpu) portable_delay_cycles(cpu_ms_2_cy(delay, f_cpu))
#define cpu_delay_us(delay, f_cpu) portable_delay_cycles(cpu_us_2_cy(delay, f_cpu))

//#define _delay_us(delay)     cpu_delay_us(delay, F_CPU)
//#define _delay_ms(delay)     cpu_delay_ms(delay, F_CPU)


extern void delay_ms(int ms);
void portable_delay_cycles(unsigned long n);
extern void _delay_ms(int ms);
extern void _delay_us(int us);
extern uint32_t get_heap_free_size1( void );
extern void init_grbl(void);
extern void draw_pos();

extern volatile char realtime;

#endif /* UTILS_H_ */