use v6;

class Lib::PDF::Filter::Predictors {

    use NativeCall;
    use Lib::PDF :libpdf;
    use Lib::PDF::Buf;

    my subset BPC of UInt where 1 | 2 | 4 | 8 | 16;
    my subset Predictor of Int where 1|2|10..15;

    sub pdf_filt_predict_decode(
        Blob $in, Blob $out,
        uint8 $predictor where Predictor,
        uint8 $colors,
        uint8 $bpc where BPC,
        uint16 $columns,
        uint16 $rows,
    )  returns uint32 is native(&libpdf) { * }

    sub pdf_filt_predict_encode(
        Blob $in, Blob $out,
        uint8 $predictor where Predictor,
        uint8 $colors,
        uint8 $bpc where BPC,
        uint16 $columns,
        uint16 $rows,
    )  returns uint32 is native(&libpdf) { * }

    sub buf-type(Numeric $_) {
        when 32 { uint32 }
        when 16 { uint16 }
        default { uint8 }
    }

    sub resample(|c) {
         Lib::PDF::Buf.resample(|c);
    }
    # post prediction functions as described in the PDF 1.7 spec, table 3.8

    #| tiff predictor (2)
    multi method encode($buf where Blob | Buf, 
                        Predictor :$Predictor! where 2,   #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        BPC  :$BitsPerComponent = 8, #| number of bits per color
                       ) {
        my $rows = (+$buf * 8) div ($Columns * $Colors * $BitsPerComponent);
        my $type = buf-type($BitsPerComponent);
        my \nums := resample( $buf, 8, $BitsPerComponent );
        my $out = nums.new;
        $out[+nums - 1] = 0
            if +nums;
	pdf_filt_predict_encode(nums, $out, $Predictor, $Colors, $BitsPerComponent, $Columns, $rows);
        resample( $out, $BitsPerComponent, 8);
    }

    multi method encode($buf is copy where Blob | Buf,
			Predictor :$Predictor! where { 10 <= $_ <= 15}, #| predictor function
			UInt :$Columns = 1,          #| number of samples per row
			UInt :$Colors = 1,           #| number of colors per sample
			BPC  :$BitsPerComponent = 8, #| number of bits per color
        ) {

        my uint $bpc = $BitsPerComponent;
        my uint $colors = $Colors;
        if $bpc > 8 {
            $colors *= $bpc div 8;
            $bpc = 8;
        }
        $buf = resample($buf, 8, $bpc);

        my uint $row-size = $colors * $Columns;
        my buf8 $out .= new;

        my $padding = do {
            my $bit-padding = -($row-size * $bpc) % 8;
            $bit-padding div $bpc;
        }

        my $rows = +$buf div $row-size;
        # preallocate, allowing room for per-row data + tag + padding
        $out[$rows * ($row-size + $padding + 1) - 1] = 0
            if $rows;

        pdf_filt_predict_encode($buf, $out, $Predictor, $colors, $bpc, $Columns, $rows);

        resample($out, $bpc, 8);
    }

    # prediction filters, see PDF 1.7 spec table 3.8
    multi method encode($buf where Blob | Buf,
			Predictor :$Predictor=1, #| predictor function
        ) {
        $buf;
    }

    # prediction filters, see PDF 1.7 spec table 3.8
    multi method decode($buf where Blob | Buf, 
                        Predictor :$Predictor! where 2  , #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        UInt :$BitsPerComponent = 8, #| number of bits per color
                       ) {
        my $rows = (+$buf * 8) div ($Columns * $Colors * $BitsPerComponent);
        my \nums := resample( $buf, 8, $BitsPerComponent );
        my $out = nums.new;
        $out[+nums - 1] = 0
            if +nums;
	pdf_filt_predict_decode(nums, $out, $Predictor, $Colors, $BitsPerComponent, $Columns, $rows);
        resample( $out, $BitsPerComponent, 8);
    }

    multi method decode($buf is copy,  #| input stream
                        Predictor :$Predictor! where { 10 <= $_ <= 15}, #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        UInt :$BitsPerComponent = 8, #| number of bits per color
        ) {

        my uint $bpc = $BitsPerComponent;
        my uint $colors = $Colors;
        if $bpc > 8 {
            $colors *= $bpc div 8;
            $bpc = 8;
        }

        my uint $row-size = $colors * $Columns;
        my $padding = do {
            my $bit-padding = -($row-size * $bpc) % 8;
            $bit-padding div $bpc;
        }
        # prepare buffers 
        $buf = resample($buf, 8, $bpc);
        my $rows = +$buf div ($row-size + $padding + 1);
        my buf8 $out .= new;
         # preallocate
        $out[$rows * $row-size - 1] = 0
            if $rows;

        pdf_filt_predict_decode($buf, $out, $Predictor, $colors, $bpc, $Columns, $rows);

        resample($out, $bpc, 8);
    }

    multi method decode($buf, Predictor :$Predictor = 1, ) is default {
        $buf;
    }

}
