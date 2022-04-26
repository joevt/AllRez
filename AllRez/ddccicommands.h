// DDC commands

oneddccmd(F1, "Identification request"   )
oneddccmd(F3, "Capabilities request"     )
oneddccmd(B1, "Display self-test request")
oneddccmd(07, "Timing request"           )
oneddccmd(01, "VCP request"              )
oneddccmd(03, "VCP set"                  )
oneddccmd(E2, "Table read request"       )
oneddccmd(E7, "Table write"              )
oneddccmd(F5, "Enable application report")
oneddccmd(0C, "Save current settings"    )
oneddccmd(E1, "Identification reply"     )
oneddccmd(E3, "Capabilities reply"       )
oneddccmd(A1, "Display self-test reply"  )
oneddccmd(06, "Timing reply"             )
oneddccmd(02, "VCP reply"                )
oneddccmd(09, "VCP reset"                )
oneddccmd(E4, "Table read reply"         )

#undef oneddccmd
