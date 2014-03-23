<?php
namespace Snape\Utility;
{

$moduleName = 'wdetect';
$className = '\WDetecter';

if (!extension_loaded($moduleName))
{
    die("module <{$moduleName}> is not loaded");
}
else if (!class_exists($className))
{
    die("module <{$moduleName}> loaded, but class <{$className}> still non exists");
}

class Detecter
{
    protected static $errors = array(
        \WDetecter::ERR_UNKNOWN => '未知错误',
        \WDetecter::ERR_WRONG_PARAM => '参数错误',
        \WDetecter::ERR_IMG_FILE_NONEXISTS => '图片文件不存在',
        \WDetecter::ERR_IMG_FILE_UNREADABLE => '无法读取图片文件内容',
        \WDetecter::ERR_IMG_FILE_EXT => '不支持的图片文件类型（扩展名）',
        \WDetecter::ERR_IMG_FILE_SIZE => '图片文件体积不合格',
        \WDetecter::ERR_IMG_TYPE => '不支持的图片文件类型（文件头）',
        \WDetecter::ERR_IMG_CONTENT => '图片文件内容有误',
        \WDetecter::ERR_IMG_SIZE => '图片尺寸不合格',
        \WDetecter::ERR_NO_MATCH => '找不到符合条件的图像',
        \WDetecter::ERR_DETECT_FAILED => '探测失败',
        \WDetecter::ERR_RECOGNIZE_FAILED => '识别失败',
        \WDetecter::ERR_CALC_CHART_WIDTH => '计算图表宽度失败',
        \WDetecter::ERR_DIGIT_X_INTERACT => '图像上的数字横向重叠(可能是数字笔画有断开)',
    );

    protected static $optsNamespaces = array(
        '\Opts\Small' => array(170, 180),
        '\Opts\Large' => array(190, 200),
    );
    private $optsNamespace;

    /**
     * @var \WDetecter
     */
    protected $wdter;

    protected $debug = false;

    public function __construct($debug = false)
    {
        $this->debug = $debug === true;
        $this->wdter = new \WDetecter;
    }

    public function detectAll($imgFile)
    {
        $prepareOpts = (array)(new \Opts\PrepareOpts);
        $prepareOpts['img_file'] = $imgFile;
        $prepareRes = $this->wdter->prepare($prepareOpts);
        if (isset($prepareRes[0]) and $prepareRes[0] === \WDetecter::OK)
        {
            $chartOpts = (array)(new \Opts\Chart);
            $locateRes = $this->wdter->locate($chartOpts);
            if ($locateRes[0] === \WDetecter::OK)
            {
                $chartWidth = $locateRes[3];
                $this->optsNamespace = null;
                foreach (static::$optsNamespaces as $namespace => &$range)
                {
                    if ($range[0] <= $chartWidth and $chartWidth <= $range[1])
                    {
                        $this->optsNamespace = $namespace;
                    }
                }
                if ($this->optsNamespace === null)
                {
                    throw new \Exception("no matching chart-width: {$chartWidth}", 10086);
                }
            }
            if ($locateRes[0] === \WDetecter::OK)
            {
                $optClasses = array(
                    'SongdaROC', 'TuwenROC', 'YuanwenROC', 'ShareROC',
                    'Songda',
                    'Tuwen', 'TuwenTimes', 'TuwenRate',
                    'Yuanwen', 'YuanwenTimes', 'YuanwenRate',
                    'Share', 'ShareTimes',
                );
                list($x0, $y0) = $locateRes[2];
                foreach ($optClasses as $optClass)
                {
                    $this->{'detect' . $optClass}($x0, $y0);
                }
            }
            else
            {
                $this->println('failed on locating: ' . static::$errors[$locateRes[0]] . PHP_EOL . 'params:');
                $this->printr($locateRes);
                $this->printr($chartOpts);
                echo str_repeat('-', 40), PHP_EOL;
            }
        }
        else
        {
            $this->println('failed on preparing: ' . static::$errors[$prepareRes[0]] . PHP_EOL . 'params:');
            $this->printr($prepareOpts);
        }
    }

    protected function detect(array $opts, $kind = '')
    {
        $kindInfo = explode('\\', $kind);
        $kind = end($kindInfo);
        $res = $this->wdter->detect($opts);
        $code = array_shift($res);
        if ($code === \WDetecter::OK)
        {
            $this->println('detected ' . $kind . ': ' . json_encode($res));
            return true;
        }
        else
        {
            $this->println('failed on detecting ' . $kind . ': ' . static::$errors[$code]);
            $this->println('params:');
            $this->printr($opts);
            echo str_repeat('-', 40), PHP_EOL;
            return false;
        }
    }

    public function __call($method, $args)
    {
        if (strpos($method, 'detect') === 0)
        {
            $optClass = $this->optsNamespace . '\\' . substr($method, strlen('detect'));
            $opts = null;
            if (class_exists($optClass))
            {
                switch (count($args))
                {
                case 0:
                    $opts = new $optClass;
                    break;
                case 1:
                    $opts = new $optClass($args[0]);
                    break;
                case 2:
                    $opts = new $optClass($args[0], $args[1]);
                    break;
                case 3:
                    $opts = new $optClass($args[0], $args[1], $args[2]);
                    break;
                default:
                    break;
                }
                if ($opts !== null)
                {
                    return $this->detect((array)$opts, $optClass);
                }
            }
        }
        throw new \Exception('failed on invoking method ' . get_class($this) . '::' . $method . '.', 10086);
    }

    protected function println($content)
    {
        if ($this->debug)
        {
            echo $content, PHP_EOL;
        }
    }

    protected function printr($arr)
    {
        if ($this->debug)
        {
            print_r($arr);
        }
        echo str_repeat('-', 40), PHP_EOL;
    }

    public static function check()
    {
        global $className;
        assert($className::OK == 0);
        assert($className::ERR_UNKNOWN == 1);
    }
}

}

namespace Opts;
{

class Options
{}

class PrepareOpts extends Options
{
    public $img_file = "";
    public $img_file_min_size = 24;
    public $img_file_max_size = 8388608;
    public $img_min_width = 160;
    public $img_max_width = 1920;
    public $img_min_height = 120;
    public $img_max_height = 1280;
}

class DetectOpts extends Options
{
    public $type;
    public $x_err = 1, $y_err = 1;
    public $inverse = false;
}

class LocateOpts extends DetectOpts
{}

// 中间左边漏斗图
class Chart extends LocateOpts
{
    public $type = \WDetecter::CMD_LOCATE_CHART;
    public $chart_min_width = 130;
    public $chart_max_width = 184;
    public $chart_min_height = 32;
    public $chart_max_height = 144;
    public $echelons = 3;
    public $echelon_padding_left = 0;
    public $chart_min_margin_right = 12, $chart_max_margin_right = 40;
    public $chart_margin_max_fg = 2;
    public $echelon_max_wrong_row = 4;
    public $echelon_gradient_min_continuous = 6;
}

}

// see 1.jpg/16.jpg
namespace Opts\Large;
use Opts;
{

class DIBOpts extends Opts\DetectOpts
{
    public $left, $top, $right, $bottom;

    public function reset($left, $right, $top, $bottom, $x0 = 0, $y0 = 0)
    {
        $this->left = $left + $x0;
        $this->right = $right + $x0;
        $this->top = $top + $y0;
        $this->bottom = $bottom + $y0;
    }

    public function adjust($x0 = 0, $y0 = 0)
    {
        $this->left += $x0;
        $this->right += $x0;
        $this->top += $y0;
        $this->bottom += $y0;
    }

    public function setPadding($padding)
    {
        $this->left -= $padding;
        $this->top -= $padding;
        $this->right += $padding;
        $this->bottom += $padding;
    }
}

class NumOpts extends DIBOpts
{
    public $digit_height = 9;
    public $digit_min_width = 4;
    public $digit_max_width = 7;
    public $circle_min_diameter_v = 2;
    public $vline_adj = 0, $hline_adj = 0;
    public $vline_max_break = 0, $hline_max_break = 0;
    public $vline_min_gap = 2, $hline_min_gap = 2;

    public $comma_max_width = 3, $comma_max_height = 4,
           $comma_min_area = 3, $comma_protrude = 2;

    public $dot_max_width = 2, $dot_max_height = 2,
           $dot_min_area = 2;
}

class IntegerOpts extends NumOpts
{
    public $type = \WDetecter::CMD_DETECT_INTEGER;
}

class PercentOpts extends NumOpts
{
    public $type = \WDetecter::CMD_DETECT_PERCENT;
    public $percent_width = 11;
}

class ROCOpts extends IntegerOpts
{
    protected $padding = 2;
    protected $width = 48;
    protected $vgap_min = 33;
    protected $vgap_max = 38;
    protected $turn = 0;
    protected $baseTop = 3;

    public $right = 335;

    public function __construct($x0 = 0, $y0 = 0)
    {
        $this->left = $this->right - $this->width;
        if ($this->turn === 1)
        {
            $this->left -= 32;
        }
        $this->top = $this->baseTop + ($this->turn - 1) * $this->vgap_min;
        $this->bottom = $this->baseTop + ($this->turn - 1) * $this->vgap_max + $this->digit_height + $this->comma_protrude;
        $this->setPadding($this->padding);
        $this->adjust($x0, $y0);
    }
}

// 送达 图表右侧
class SongdaROC extends ROCOpts
{
    public $turn = 1;
}

// 图文 图表右侧
class TuwenROC extends ROCOpts
{
    public $turn = 2;
}

// 原文 图表右侧
class YuanwenROC extends ROCOpts
{
    public $turn = 3;
}

// 分享 图表右侧
class ShareROC extends ROCOpts
{
    public $turn = 4;
}

class TabAssist
{
    protected static $offsetTop = 269;
    protected static $padding = 3;

    public static function populate($obj, $x0 = 0, $y0 = 0)
    {
        $obj->left += $x0;
        $obj->right += $x0;
        $obj->top = $y0 + static::$offsetTop;
        $obj->bottom = $obj->top + $obj->digit_height;
        if ($obj instanceof IntegerOpts)
        {
            $obj->bottom += $obj->comma_protrude;
        }
        $obj->setPadding(static::$padding);
    }
}

// 送达 人数
class Songda extends IntegerOpts
{
    public $left = 3;
    public $right = 93;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 人数
class Tuwen extends IntegerOpts
{
    public $left = 93;
    public $right = 168;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 次数
class TuwenTimes extends IntegerOpts
{
    public $left = 168;
    public $right = 247;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 转化率
class TuwenRate extends PercentOpts
{
    public $left = 247;
    public $right = 345;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 人数
class Yuanwen extends IntegerOpts
{
    public $left = 345;
    public $right = 410;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 次数
class YuanwenTimes extends IntegerOpts
{
    public $left = 410;
    public $right = 479;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 转化率
class YuanwenRate extends PercentOpts
{
    public $left = 479;
    public $right = 586;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 分享 人数
class Share extends IntegerOpts
{
    public $left = 586;
    public $right = 650;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 分享 次数
class ShareTimes extends IntegerOpts
{
    public $left = 650;
    public $right = 715;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

}

// see 2.jpg
namespace Opts\Small;
use Opts;
{

class DIBOpts extends Opts\DetectOpts
{
    public $left, $top, $right, $bottom;

    public function reset($left, $right, $top, $bottom, $x0 = 0, $y0 = 0)
    {
        $this->left = $left + $x0;
        $this->right = $right + $x0;
        $this->top = $top + $y0;
        $this->bottom = $bottom + $y0;
    }

    public function adjust($x0 = 0, $y0 = 0)
    {
        $this->left += $x0;
        $this->right += $x0;
        $this->top += $y0;
        $this->bottom += $y0;
    }

    public function setPadding($padding)
    {
        $this->left -= $padding;
        $this->top -= $padding;
        $this->right += $padding;
        $this->bottom += $padding;
    }
}

class NumOpts extends DIBOpts
{
    public $digit_height = 9;
    public $digit_min_width = 4;
    public $digit_max_width = 7;
    public $circle_min_diameter_v = 2;
    public $vline_adj = 0, $hline_adj = 0;
    public $vline_max_break = 0, $hline_max_break = 0;
    public $vline_min_gap = 2, $hline_min_gap = 2;

    public $comma_max_width = 3, $comma_max_height = 4,
           $comma_min_area = 3, $comma_protrude = 2;

    public $dot_max_width = 2, $dot_max_height = 2,
           $dot_min_area = 2;
}

class IntegerOpts extends NumOpts
{
    public $type = \WDetecter::CMD_DETECT_INTEGER;
}

class PercentOpts extends NumOpts
{
    public $type = \WDetecter::CMD_DETECT_PERCENT;
    public $percent_width = 11;
}

class ROCOpts extends IntegerOpts
{
    protected $padding = 2;
    protected $width = 36;
    protected $vgap_min = 36;
    protected $vgap_max = 37;
    protected $turn = 0;
    protected $baseTop = 3;

    public $right = 301;

    public function __construct($x0 = 0, $y0 = 0)
    {
        $this->left = $this->right - $this->width;
        if ($this->turn === 1)
        {
            $this->left -= 32;
        }
        $this->top = $this->baseTop + ($this->turn - 1) * $this->vgap_min;
        $this->bottom = $this->baseTop + ($this->turn - 1) * $this->vgap_max + $this->digit_height + $this->comma_protrude;
        $this->setPadding($this->padding);
        $this->adjust($x0, $y0);
    }
}

// 送达 图表右侧
class SongdaROC extends ROCOpts
{
    public $turn = 1;
}

// 图文 图表右侧
class TuwenROC extends ROCOpts
{
    public $turn = 2;
}

// 原文 图表右侧
class YuanwenROC extends ROCOpts
{
    public $turn = 3;
}

// 分享 图表右侧
class ShareROC extends ROCOpts
{
    public $turn = 4;
}

class TabAssist
{
    protected static $offsetTop = 241;
    protected static $padding = 3;

    public static function populate($obj, $x0 = 0, $y0 = 0)
    {
        $obj->left += $x0;
        $obj->right += $x0;
        $obj->top = $y0 + static::$offsetTop;
        $obj->bottom = $obj->top + $obj->digit_height;
        if ($obj instanceof IntegerOpts)
        {
            $obj->bottom += $obj->comma_protrude;
        }
        $obj->setPadding(static::$padding);
    }
}

// 送达 人数
class Songda extends IntegerOpts
{
    public $left = 3;
    public $right = 84;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 人数
class Tuwen extends IntegerOpts
{
    public $left = 84;
    public $right = 150;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 次数
class TuwenTimes extends IntegerOpts
{
    public $left = 150;
    public $right = 215;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 图文 转化率
class TuwenRate extends PercentOpts
{
    public $left = 215;
    public $right = 313;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 人数
class Yuanwen extends IntegerOpts
{
    public $left = 313;
    public $right = 370;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 次数
class YuanwenTimes extends IntegerOpts
{
    public $left = 370;
    public $right = 428;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 原文 转化率
class YuanwenRate extends PercentOpts
{
    public $left = 428;
    public $right = 526;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 分享 人数
class Share extends IntegerOpts
{
    public $left = 526;
    public $right = 584;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

// 分享 次数
class ShareTimes extends IntegerOpts
{
    public $left = 584;
    public $right = 643;

    public function __construct($x0 = 0, $y0 = 0)
    {
        TabAssist::populate($this, $x0, $y0);
    }
}

}

if ($GLOBALS['argc'])
{
    $imgFile = $GLOBALS['argc'] > 1 ? $GLOBALS['argv'][1] : "/data/fsuggest/wdetect.forgiven/data/16.jpg";
    $detecter = new \Snape\Utility\Detecter(true);
    $detecter->check();
    $detecter->detectAll($imgFile);
}
