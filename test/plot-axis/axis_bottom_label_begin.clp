(layer/resize 1024px 512px)
(layer/set-dpi 96)

(figure/plot
    margin 8em
    axis (
        align bottom
        limit (1451606400 1451610000)
        label-placement (subdivide 4)
        label-format (datetime "%H:%M:%S")
        label-attach left
        label-rotate 90))
