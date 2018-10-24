use v6;

class PDF::Native::Filter::Predictors {

    use NativeCall;
    use PDF::Native :libpdf;
    use PDF::Native::Buf :pack;

    my subset BPC of UInt where 1|2|4|8|16|32;

    constant None = 1;
    constant TIFF = 2;
    constant PNG = 10;
    constant PNG-Range = 10 .. 15;

    subset Predictor of Int where None | TIFF | PNG-Range;

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

    # post prediction functions as described in the PDF 1.7 spec, table 3.8

    #| tiff predictor (2)
    multi method encode($buf where Blob,
                        Predictor :$Predictor! where TIFF, #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        BPC  :$BitsPerComponent = 8, #| number of bits per color
                       ) {
        my $rows = ($buf.bytes * 8) div ($Columns * $Colors * $BitsPerComponent);
        my \nums := unpack( $buf, $BitsPerComponent );
        my $out = nums.WHAT.allocate(nums.elems);
	pdf_filt_predict_encode(nums, $out, $Predictor, $Colors, $BitsPerComponent, $Columns, $rows);
        $out = pack($out, $BitsPerComponent);
	$out;
    }

    multi method encode($buf is copy where Blob,
			Predictor :$Predictor! where PNG-Range, #| predictor function
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
        my uint $row-size = ($colors * $bpc * $Columns + 7) div 8;
        my $rows = +$buf div $row-size;
        # preallocate, allowing room for per-row data + tag + padding
        my buf8 $out = buf8.allocate($rows * ($row-size + 1));

        pdf_filt_predict_encode($buf, $out, $Predictor, $colors, $bpc, $Columns, $rows);

        $out;
    }

    # prediction filters, see PDF 1.7 spec table 3.8
    multi method encode($buf where Blob, Predictor :$Predictor = None,
			UInt :$Columns=1, UInt :$Colors=1,
			BPC :$BitsPerComponent=8,
        ) is default {
        $buf;
    }

    # prediction filters, see PDF 1.7 spec table 3.8
    multi method decode($buf where Blob,
                        Predictor :$Predictor! where TIFF, #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        BPC :$BitsPerComponent = 8,  #| number of bits per color
                       ) {
        my $rows = ($buf.bytes * 8) div ($Columns * $Colors * $BitsPerComponent);
        my \nums := unpack( $buf, $BitsPerComponent );
        my $out = nums.WHAT.allocate(nums.elems);
	pdf_filt_predict_decode(nums, $out, $Predictor, $Colors, $BitsPerComponent, $Columns, $rows);
        $out = pack( $out, $BitsPerComponent);
	$out;
    }

    multi method decode(Blob $buf,  #| input stream
                        Predictor :$Predictor! where PNG-Range, #| predictor function
                        UInt :$Columns = 1,          #| number of samples per row
                        UInt :$Colors = 1,           #| number of colors per sample
                        BPC :$BitsPerComponent = 8,  #| number of bits per color
        ) {

        my uint $bpc = $BitsPerComponent;
        my uint $colors = $Colors;
        if $bpc > 8 {
            $colors *= $bpc div 8;
            $bpc = 8;
        }

        my uint $row-size = ($colors * $bpc * $Columns + 7) div 8;
        my $rows = +$buf div ($row-size + 1);
        my buf8 $out = buf8.allocate($rows * $row-size);

        pdf_filt_predict_decode($buf, $out, $Predictor, $colors, $bpc, $Columns, $rows);

        $out;
    }

    multi method decode($buf, Predictor :$Predictor = None,
			UInt :$Columns=1, UInt :$Colors=8,
			BPC :$BitsPerComponent=8 ) is default {
        $buf;
    }

}
