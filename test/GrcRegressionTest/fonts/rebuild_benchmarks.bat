grcompiler -D -v2 SchMain.gdl SchInput.ttf SchBenchmark.ttf
copy SchBenchmark.ttf dbg_Sch
copy SchBenchmark.gdx dbg_Sch
copy $_temp.gdl dbg_Sch
copy gdlerr.txt dbg_Sch
copy dbg_*.txt dbg_Sch

grcompiler -D -v2 CharisMain.gdl CharisInput.ttf CharisBenchmark.ttf
copy CharisBenchmark.ttf dbg_Charis
copy CharisBenchmark.gdx dbg_Charis
copy $_temp.gdl dbg_Charis
copy gdlerr.txt dbg_Charis
copy dbg_*.txt dbg_Charis

grcompiler -D -v2 -p PigLatinMain.gdl PigLatinInput.ttf PigLatinBenchmark_v2.ttf
copy PigLatinBenchmark_v2.ttf dbg_PigLatin_v2
copy PigLatinBenchmark_v2.gdx dbg_PigLatin_v2
copy $_temp.gdl dbg_PigLatin_v2
copy gdlerr.txt dbg_PigLatin_v2
copy dbg_*.txt dbg_PigLatin_v2

grcompiler -D -v3 PigLatinMain.gdl PigLatinInput.ttf PigLatinBenchmark_v3.ttf
copy PigLatinBenchmark_v3.ttf dbg_PigLatin_v3
copy PigLatinBenchmark_v3.gdx dbg_PigLatin_v3
copy $_temp.gdl dbg_PigLatin_v3
copy gdlerr.txt dbg_PigLatin_v3
copy dbg_*.txt dbg_PigLatin_v3
    
grcompiler -D -v3 -p -offsets PadaukMain.gdl PadaukInput.ttf PadaukBenchmark_v3.ttf
copy PadaukBenchmark_v3.ttf dbg_Padauk
copy PadaukBenchmark_v3.gdx dbg_Padauk
copy $_temp.gdl dbg_Padauk
copy gdlerr.txt dbg_Padauk
copy dbg_*.txt dbg_Padauk
