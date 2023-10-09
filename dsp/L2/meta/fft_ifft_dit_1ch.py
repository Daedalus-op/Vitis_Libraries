from aie_common import *
import json
import sys

# fft_ifft_dit_1ch.hpp:821:    static_assert(fnCheckDataType<TT_DATA>(), "ERROR: TT_IN_DATA is not a supported type");
# fft_ifft_dit_1ch.hpp:822:    static_assert(fnCheckDataType<TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:823:    static_assert(fnCheckDataIOType<TT_DATA, TT_OUT_DATA>(), "ERROR: TT_OUT_DATA is not a supported type");
#   ignore this internal debug aid
# fft_ifft_dit_1ch.hpp:824:    static_assert(fnCheckTwiddleType<TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is not a supported type");
# fft_ifft_dit_1ch.hpp:825:    static_assert(fnCheckDataTwiddleType<TT_DATA, TT_TWIDDLE>(), "ERROR: TT_TWIDDLE is incompatible with data type");
# fft_ifft_dit_1ch.hpp:826:    static_assert(fnCheckPointSize<TP_POINT_SIZE>(),
# fft_ifft_dit_1ch.hpp:828:    static_assert(TP_FFT_NIFFT == 0 || TP_FFT_NIFFT == 1, "ERROR: TP_FFT_NIFFT must be 0 (reverse) or 1 (forward)");
# fft_ifft_dit_1ch.hpp:829:    static_assert(fnCheckShift<TP_SHIFT>(), "ERROR: TP_SHIFT is out of range (0 to 60)");
# fft_ifft_dit_1ch.hpp:830:    static_assert(fnCheckShiftFloat<TT_DATA, TP_SHIFT>(),
# fft_ifft_dit_1ch.hpp:832:    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE");
# fft_ifft_dit_1ch.hpp:833:    static_assert(TP_WINDOW_VSIZE / TP_POINT_SIZE >= 1, "ERROR: TP_WINDOW_VSIZE must be a multiple of TP_POINT_SIZE")
# fft_ifft_dit_1ch.hpp:834:    static_assert((TP_DYN_PT_SIZE == 0) || (TP_POINT_SIZE != 16),
# fft_ifft_dit_1ch_graph.hpp:152:        static_assert(fnCheckCascLen<TT_DATA, TP_END_RANK, TP_CASC_LEN>(), "Error: TP_CASC_LEN is invalid");
# fft_ifft_dit_1ch_graph.hpp:153:        static_assert(fnCheckCascLen2<TT_DATA, TP_POINT_SIZE, TP_CASC_LEN>(), "Error: 16 point float FFT does not support cascade")
# fft_ifft_dit_1ch_graph.hpp:842:    static_assert(TP_API == kStreamAPI, "Error: Only Stream interface is supported for parallel FFT");
# fft_ifft_dit_1ch_graph.hpp:843:    static_assert(TP_PARALLEL_POWER >= 1 && TP_PARALLEL_POWER < 9,

TP_POINT_SIZE_min = 16
TP_POINT_SIZE_max = 65536
TP_WINDOW_VSIZE_min = 16
TP_CASC_LEN_min = 1
TP_CASC_LEN_max = 11
TP_SHIFT_min=0
TP_SHIFT_max=60
TP_RND_min = 0
TP_RND_max = 7
#TP_FFT_NIFFT_min=0
#TP_FFT_NIFFT_max=1
#TP_API_min=0
#TP_API_max=1
#AIE_VARIANT_min=2
#AIE_VARIANT_max=1

def fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE):
  validTypeCombos = [
      ("cint16", "cint16"),
      ("cint32", "cint16"),
      ("cfloat", "cfloat")
    ]
  return (
    isValid if ((TT_DATA,TT_TWIDDLE) in validTypeCombos)
    else (
    isError(f"Invalid Data/Twiddle type combination ({TT_DATA},{TT_TWIDDLE}). ")
    )
  )

def validate_TT_TWIDDLE(args):
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  return fn_validate_twiddle_type(TT_DATA, TT_TWIDDLE)

#https://stackoverflow.com/a/57025941
#  every power of 2 has exactly 1 bit set to 1 (the bit in that number's log base-2 index).
# So when subtracting 1 from it, that bit flips to 0 and all preceding bits flip to 1.
# That makes these 2 numbers the inverse of each other so when AND-ing them, we will
#  get 0 as the result.
def fn_is_power_of_two(n):
  return (
    (n & (n-1) == 0) and n!=0 )

# good candidate for aie_common, especially if we want to give better error messages
# finds the first instance of a given paramter name in the list of parameters.
# Retuns None if can't find it

def fn_validate_point_size(TP_POINT_SIZE, TP_DYN_PT_SIZE, TT_DATA, TP_PARALLEL_POWER, TP_API):
  if TP_POINT_SIZE < TP_POINT_SIZE_min or TP_POINT_SIZE > TP_POINT_SIZE_max :
        return isError(f"Minimum and maximum value for Point Size is {TP_POINT_SIZE_min} and {TP_POINT_SIZE_max},respectively, but got {TP_POINT_SIZE}.")
  checkPointSizeIsPowerOf2 = isValid if fn_is_power_of_two(TP_POINT_SIZE) else (
    isError(f"Point size ({TP_POINT_SIZE}) must be a power of 2")
  )

  minPointSize = TP_POINT_SIZE_min
  # You can't switch between multiple point sizes if the max point size specified is also the minimum supported..
  checkDynPointSizeIsMoreThanMinPointSize = (
    isError(f"Point size ({TP_POINT_SIZE}) must be higher than the minimum suppored size ({minPointSize}) when using dynamic point FFT.")
    if (TP_DYN_PT_SIZE == 1 and TP_POINT_SIZE <= minPointSize)
    else isValid
  )

  # Check for max FFT point size per kernel.
  # Allow for 4k cint16. May require single buffer constraint to map with IO buffer interface.
  # Allow for 4k cint32, as buffer reuse optimization results in reduced memory footprint
  # Allow for 2k for cfloat.
  checkMaxPointSizePerKernel = isValid
  if (TT_DATA=="cint16"):
    if (TP_API == 0):
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 2048):
        # Allow for 4k FFT with IO buffer.
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 2048 for cint16 with Windowed interfaces ")
    else:
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 4096):
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 4096 for cint16 with Streaming interfaces ")
  elif (TT_DATA=="cint32"):
    if (TP_API == 0):
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 2048):
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 2048 for cint32 with Windowed interfaces ")
    else:
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 4096):
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 4096 for cint32 with Streaming interfaces ")
  else:
    if (TP_API == 0):
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 1024):
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 1024 for cfloat with Windowed interfaces ")
    else:
      if ((TP_POINT_SIZE>>TP_PARALLEL_POWER) > 2048):
        checkMaxPointSizePerKernel = isError(f"Point size per kernel cannot exceed 2048 for cfloat with Streaming interfaces ")

  for check in (checkPointSizeIsPowerOf2,checkDynPointSizeIsMoreThanMinPointSize, checkMaxPointSizePerKernel):
    if check["is_valid"] == False :
      return check
  return isValid

def fn_validate_round_val(TP_RND, AIE_VARIANT):
  if AIE_VARIANT == 1:
    if TP_RND == k_rnd_mode_map_aie1["rnd_ceil"] or TP_RND == k_rnd_mode_map_aie1["rnd_floor"]:
      return isError(f"This value of round mode is not supported for FFT. For the targeted AIE-ML device, supported values are \n2: rnd_pos_inf,\n3: rnd_neg_inf,\n4: rnd_sym_zero,\n5: rnd_sym_inf,\n6: rnd_conv_even,\n7: rnd_conv_odd")
  elif AIE_VARIANT == 2:
    if TP_RND == k_rnd_mode_map_aie2["rnd_ceil"] or TP_RND == k_rnd_mode_map_aie2["rnd_floor"] or TP_RND == k_rnd_mode_map_aie2["rnd_sym_floor"] or TP_RND == k_rnd_mode_map_aie2["rnd_sym_ceil"]:
      return isError(f"This value of round mode is not supported for the FFT. For the targeted AIE device, supported values are \n 8: rnd_neg_inf,\n9: rnd_pos_inf,\n10: rnd_sym_zero,\n11: rnd_sym_inf,\n12: rnd_conv_even,\n13: rnd_conv_odd")
  return isValid

def validate_TP_POINT_SIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TT_DATA = args["TT_DATA"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_API = args["TP_API"]
  return fn_validate_point_size(TP_POINT_SIZE, TP_DYN_PT_SIZE, TT_DATA, TP_PARALLEL_POWER, TP_API)


def validate_TP_SHIFT(args):
  TP_SHIFT = args["TP_SHIFT"]
  TT_DATA = args["TT_DATA"]
  return fn_validate_shift_val(TT_DATA, TP_SHIFT)

def validate_TP_RND(args):
  TP_RND = args["TP_RND"]
  AIE_VARIANT = args["AIE_VARIANT"]
  return fn_validate_round_val(TP_RND, AIE_VARIANT)

def fn_validate_shift_val(TT_DATA, TP_SHIFT):
  if TP_SHIFT< TP_SHIFT_min or TP_SHIFT > TP_SHIFT_max:
	    return isError(f"Minimum and Maximum value for parameter Shift is {TP_SHIFT_min} and {TP_SHIFT_max},respectively, but got {TP_SHIFT}. ")
  return fn_float_no_shift(TT_DATA, TP_SHIFT)


#assumes n is a power of 2
def fn_log2(n):
  original_n = n
  if not fn_is_power_of_two(n):
    sys.exit("invalid assumption that n is a power of two")
  if n != 0:
    power_cnt = 0
    while n % 2 == 0:
      # keep right shifting until the power of two bit is at the LSB.
      n = n >> 1
      power_cnt+=1
      #print(f"n={n} and iter={power_cnt}")
      if n == 0 :
        sys.exit(f"Something went wrong when log2 {original_n}")
    return power_cnt
  else:
    sys.exit("Can't log2 0")
    #return Inf

def fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_CASC_LEN):
  if TP_CASC_LEN < TP_CASC_LEN_min or TP_CASC_LEN > TP_CASC_LEN_max :
        return isError(f"Minimum and maximum value for cascade length is {TP_CASC_LEN_min} and {TP_CASC_LEN_max},respectively, but got {TP_CASC_LEN}.")
  # Defines how many radix-2 ranks there are in the FFT itself (subframe or main FFT).
  log2PointSize = fn_log2(TP_POINT_SIZE>>TP_PARALLEL_POWER)
  # equation for integer ffts is complicated by the fact that odd power of 2 point sizes start with a radix 2 stage
  TP_END_RANK = CEIL(log2PointSize, 2) if TT_DATA != "cfloat" else log2PointSize

  checkCascLenIsNotGreaterThanRanks = (
    isValid if (TP_CASC_LEN <= TP_END_RANK) else
    isError(f"The cascade length provided ({TP_CASC_LEN}) exceeds the maximum of ({TP_END_RANK}) for the given point size ({TP_POINT_SIZE})")
  )

  return checkCascLenIsNotGreaterThanRanks

def validate_TP_CASC_LEN(args):
  TT_DATA = args["TT_DATA"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  return fn_validate_casc_len(TT_DATA, TP_POINT_SIZE, TP_PARALLEL_POWER, TP_CASC_LEN)


def fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE):
  # Disable the window_vsize check for dynamic point size, due to incorrectly created caller function's arguments.
  if TP_WINDOW_VSIZE < TP_WINDOW_VSIZE_min:
	    return isError(f"Minimum value for Input size is {TP_WINDOW_VSIZE_min}, but got {TP_WINDOW_VSIZE}.")
  if (TP_DYN_PT_SIZE == 1) :
   return isValid

  if (TP_WINDOW_VSIZE % TP_POINT_SIZE != 0):
    return isError(f"Input window size ({TP_WINDOW_VSIZE}) must be a multiple of point size ({TP_POINT_SIZE}) ")
  return isValid


def validate_TP_WINDOW_VSIZE(args):
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  return fn_validate_window_size(TP_POINT_SIZE, TP_WINDOW_VSIZE, TP_DYN_PT_SIZE)



def fn_validate_parallel_power(TP_API, TP_PARALLEL_POWER):
#as of 23.1, iobuffer (window) API is supported with TP_PARALLEL_POWER>=1
#  if (TP_API == 0 and TP_PARALLEL_POWER >=1 ):
#    return isError("Only stream interface is supported for parallel FFT.")
  return isValid

def validate_TP_PARALLEL_POWER(args):
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  return fn_validate_parallel_power(TP_API, TP_PARALLEL_POWER)

def validate_TP_SAT(args):
  TP_SAT = args["TP_SAT"]
  return fn_validate_satMode(TP_SAT)



  ######### Finished Validation ###########

  ######### Updators ###########

  ######### Graph Generator ############

# Used by higher layer software to figure out how to connect blocks together.

def get_dyn_pt_port_info(portname, dir, TT_DATA, windowVSize, vectorLength=None, marginSize=0, TP_API=0):
  return [{
    "name" : f"{portname}[{idx}]" if vectorLength else f"{portname}", # portname no index
    "type" : "window" if TP_API==0 else "stream",
    "direction" : f"{dir}",
    "data_type" : TT_DATA,
    "fn_is_complex" : fn_is_complex(TT_DATA),
    "window_size" : fn_input_window_size(windowVSize, TT_DATA) + 32,
    "margin_size": marginSize
  } for idx in range((vectorLength if vectorLength else 1))] # do just one port if vectorLength=None

def info_ports(args):
  """Standard function creating a static dictionary of information
  for upper software to correctly connect the IP.
  Some IP has dynamic number of ports according to parameter set,
  so port information has to be implemented as a function"""
  TT_DATA = args["TT_DATA"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_PARALLEL_POWER=args["TP_PARALLEL_POWER"]
  TP_API = args["TP_API"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  marginSize = 0

  if TP_API == 0 and TP_DYN_PT_SIZE == 0:
    in_ports = get_port_info("in", "in", TT_DATA, TP_WINDOW_VSIZE, 1, 0, TP_API)
    out_ports = get_port_info("out", "out", TT_DATA, TP_WINDOW_VSIZE, 1, 0, TP_API)
  elif TP_API == 0 and TP_DYN_PT_SIZE == 1:
    in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
    out_ports = get_dyn_pt_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER)), 2**TP_PARALLEL_POWER, 0, TP_API)
  elif TP_API == 1 and TP_DYN_PT_SIZE == 1:
    in_ports = get_dyn_pt_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER + 1)), 2**(TP_PARALLEL_POWER + 1), 0, TP_API)
    out_ports = get_dyn_pt_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER + 1)), 2**(TP_PARALLEL_POWER + 1), 0, TP_API)
  else:
    in_ports = get_port_info("in", "in", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
    out_ports = get_port_info("out", "out", TT_DATA, (TP_WINDOW_VSIZE/2**(TP_PARALLEL_POWER+1)), 2**(TP_PARALLEL_POWER+1), 0, 1)
  return in_ports + out_ports

def generate_graph(graphname, args):

  if graphname == "":
    graphname = "default_graphname"
  TT_DATA = args["TT_DATA"]
  TT_TWIDDLE = args["TT_TWIDDLE"]
  TP_POINT_SIZE = args["TP_POINT_SIZE"]
  TP_FFT_NIFFT = args["TP_FFT_NIFFT"]
  TP_SHIFT = args["TP_SHIFT"]
  TP_CASC_LEN = args["TP_CASC_LEN"]
  TP_DYN_PT_SIZE = args["TP_DYN_PT_SIZE"]
  TP_WINDOW_VSIZE = args["TP_WINDOW_VSIZE"]
  TP_API = args["TP_API"]
  TP_PARALLEL_POWER = args["TP_PARALLEL_POWER"]
  TP_USE_WIDGETS = 0
  TP_RND = args["TP_RND"]
  TP_SAT = args["TP_SAT"]

  if TP_API == 0 and TP_DYN_PT_SIZE == 0:
    ssr = 1
  elif TP_API == 0 and TP_DYN_PT_SIZE == 1:
    ssr = 2**(TP_PARALLEL_POWER)
  else:
    ssr = 2**(TP_PARALLEL_POWER+1)

  code = (
f"""
class {graphname} : public adf::graph {{
public:
  // ports
  template <typename dir>
  using ssr_port_array = std::array<adf::port<dir>, {ssr}>;

  ssr_port_array<input> in;
  ssr_port_array<output> out;


  xf::dsp::aie::fft::dit_1ch::fft_ifft_dit_1ch_graph<
    {TT_DATA}, // TT_DATA
    {TT_TWIDDLE}, // TT_TWIDDLE
    {TP_POINT_SIZE}, // TP_POINT_SIZE
    {TP_FFT_NIFFT}, // TP_FFT_NIFFT
    {TP_SHIFT}, // TP_SHIFT
    {TP_CASC_LEN}, // TP_CASC_LEN
    {TP_DYN_PT_SIZE}, // TP_DYN_PT_SIZE
    {TP_WINDOW_VSIZE}, // TP_WINDOW_VSIZE
    {TP_API}, // TP_API
    {TP_PARALLEL_POWER}, // TP_PARALLEL_POWER
    {TP_USE_WIDGETS}, // TP_USE_WIDGETS
    {TP_RND}, // TP_RND
    {TP_SAT} // TP_SAT
  > fft_graph;

  {graphname}() : fft_graph() {{
    for (int i=0; i < {ssr}; i++) {{
      adf::connect<> net_in(in[i], fft_graph.in[i]);
      adf::connect<> net_out(fft_graph.out[i], out[i]);
    }}
  }}
}};
"""
  )
  out = {}
  out["graph"] = code
  out["port_info"] = info_ports(args)
  out["headerfile"] = "fft_ifft_dit_1ch_graph.hpp"
  out["searchpaths"] = [
       "L2/include/aie",
       "L2/tests/aie/common/inc",
       "L1/include/aie",
       "L1/src/aie",
       "L1/tests/aie/inc",
       "L1/tests/aie/src"
  ]
  return out

