package org.pescuma.miricogen;

import static java.lang.Math.*;
import static org.pescuma.miricogen.FileUtils.*;
import static org.pescuma.miricogen.ImageUtils.*;

import java.awt.image.BufferedImage;
import java.awt.image.DirectColorModel;
import java.awt.image.IndexColorModel;
import java.awt.image.WritableRaster;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileFilter;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Properties;

import net.sf.image4j.codec.ico.ICODecoder;
import net.sf.image4j.codec.ico.ICOEncoder;

import org.apache.tools.ant.taskdefs.Copy;
import org.apache.tools.ant.taskdefs.ExecTask;
import org.apache.velocity.Template;
import org.apache.velocity.VelocityContext;
import org.apache.velocity.app.Velocity;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.pescuma.jfg.gui.swt.JfgFormComposite;
import org.pescuma.jfg.gui.swt.JfgFormData;
import org.pescuma.jfg.reflect.ReflectionGroup;
import org.pescuma.velocity.VelocityLogger;
import org.pescuma.velocity.VelocityResourceLoader;

public class Main
{
	private static Config cfg;
	private static Text out;
	
	public static class Paths
	{
		public File protosPath = new File("protos");
		public File overlaysPath = new File("overlays");
		public File pluginsPath = new File("plugins");
		public File outPath = new File("out");
	}
	
	public static class DllInfo
	{
		public String name = "Miranda Icon Pack";
		public String description = "Miranda Icon Pack for %proto%";
		@SuppressWarnings("deprecation")
		public String copyright = "Copyright © Author Name, " + (new Date().getYear() + 1900);
		public String version = "1.0.0.0";
	}
	
	public static class Config
	{
		public final Paths paths = new Paths();
		public final DllInfo dllInfo = new DllInfo();
	}
	
	public static void main(String[] args)
	{
		initVelocity();
		
		cfg = new Config();
		Display display = new Display();
		final Shell shell = new Shell(display);
		shell.setLayout(new GridLayout(1, true));
		
		final JfgFormComposite form = new JfgFormComposite(shell, SWT.NONE, new JfgFormData(JfgFormData.DIALOG));
		form.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		form.addContentsFrom(new ReflectionGroup(cfg));
		
		Button ok = new Button(shell, SWT.PUSH);
		ok.setLayoutData(new GridData(GridData.HORIZONTAL_ALIGN_CENTER));
		ok.addListener(SWT.Selection, new Listener() {
			public void handleEvent(Event event)
			{
				form.copyToModel();
				
				try
				{
					process();
				}
				catch (Exception e)
				{
					e.printStackTrace();
					MessageBox mb = new MessageBox(shell, SWT.OK | SWT.ERROR);
					mb.setText("Error");
					mb.setMessage(e.getMessage());
					mb.open();
				}
			}
		});
		ok.setText("Generate");
		
		out = new Text(shell, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
		out.setLayoutData(new GridData(GridData.FILL_BOTH));
		
		shell.setText("Miranda Ico Generator Config");
		shell.setDefaultButton(ok);
		shell.pack();
		shell.setSize(500, 500);
		shell.open();
		while (!shell.isDisposed())
		{
			if (!display.readAndDispatch())
				display.sleep();
		}
		display.dispose();
	}
	
	protected static void process() throws IOException, InterruptedException
	{
		out.append("Starting process...\n\n");
		
		List<String> protos = readProtos();
		List<Overlay> overlays = readOverlays();
		List<Plugin> plugins = readPlugins();
		
		for (String proto : protos)
		{
			out.append(proto + "\n");
			
			for (Overlay overlay : overlays)
			{
				out.append("   - " + overlay.getName() + " : ");
				
				File outIco = getOutIco(proto, overlay);
				
				File orig = getProtoIco(proto + "-" + overlay.getName());
				if (orig.exists())
				{
					out.append("found in prots folder\n");
					copy(outIco, orig);
					continue;
				}
				
				File protoIco = getProtoIco(proto);
				if (!protoIco.exists())
				{
					out.append("could not find " + getFullPath(protoIco) + "\n");
					continue;
				}
				
				if (overlay.isEmpty())
				{
					out.append("could not find overlay, using original icon\n");
					copy(outIco, protoIco);
					continue;
				}
				
				out.append("merging icons\n");
				createIco(outIco, protoIco, overlay);
			}
			
			out.append("   - Building dll\n");
			buildDll(proto, overlays);
			
			out.append("\n");
		}
		
		out.append("Building plugins...\n\n");
		
		for (Plugin plugin : plugins)
		{
			out.append(plugin.getName() + "\n");
			out.append("   - Building dll\n");
			buildDll(plugin);
			
			out.append("\n");
		}
		
		out.append("Building preview\n");
		
		buildPreview(getPreviewFile(), protos, overlays, plugins);
		
		out.append("\n");
		out.append("Done");
	}
	
	private static File getPreviewFile()
	{
		return new File(cfg.paths.outPath, "preview.png");
	}
	
	private static void buildPreview(File preview, List<String> protos, List<Overlay> overlays, List<Plugin> plugins) throws IOException
	{
		List<String> titles = new ArrayList<String>();
		titles.add(cfg.dllInfo.name + " " + cfg.dllInfo.version);
		titles.add(cfg.dllInfo.copyright);
		
		int ICO_SIZE = 16;
		int SPACE = 4;
		int BORDER = 10;
		int FONT_SIZE = 10;
		boolean FONT_BOLD = false;
		int TITLE_SPACE = 20;
		int TITLE_FONT_SIZE = 10;
		boolean TITLE_FONT_BOLD = true;
		
		Point maxTextSize = getMaxTextSize(protos, FONT_SIZE, FONT_BOLD);
		int TEXT_MAX_WIDTH = maxTextSize.x + BORDER;
		int TEXT_HEIGHT = maxTextSize.y;
		int LINE_HEIGHT = max(TEXT_HEIGHT, ICO_SIZE);
		
		maxTextSize = getMaxTextSize(titles, TITLE_FONT_SIZE, TITLE_FONT_BOLD);
		int TITLE_HEIGHT = maxTextSize.y;
		
		Display display = Display.getCurrent();
		
		int pluginsSpace = 0;
		for (Plugin plugin : plugins)
		{
			int rows = (int) ceil(plugin.getIcons().size() / (double) overlays.size());
			pluginsSpace += rows * (LINE_HEIGHT + SPACE);
		}
		
		Image img = new Image(display, BORDER + TEXT_MAX_WIDTH + overlays.size() * (ICO_SIZE + SPACE) + BORDER, BORDER + TITLE_HEIGHT * 2
				+ SPACE + TITLE_SPACE + protos.size() * (LINE_HEIGHT + SPACE) + pluginsSpace - SPACE + BORDER);
		
		GC gc = new GC(img);
		gc.setBackground(display.getSystemColor(SWT.COLOR_WHITE));
		
		int top = BORDER;
		
		Font titleFont = setFont(gc, TITLE_FONT_SIZE, TITLE_FONT_BOLD);
		
		for (String text : titles)
		{
			int textHeight = gc.stringExtent(text).y;
			gc.drawText(text, BORDER, top + (LINE_HEIGHT - textHeight) / 2, true);
			top += TITLE_HEIGHT + SPACE;
		}
		top += TITLE_SPACE - SPACE;
		
		Font font = setFont(gc, FONT_SIZE, FONT_BOLD);
		titleFont.dispose();
		
		for (String proto : protos)
		{
			int left = BORDER;
			
			int textHeight = gc.stringExtent(proto).y;
			gc.drawText(proto, left, top + (LINE_HEIGHT - textHeight) / 2, true);
			
			left += TEXT_MAX_WIDTH + SPACE;
			
			for (Overlay overlay : overlays)
			{
				List<BufferedImage> protoImgs = ICODecoder.read(getOutIco(proto, overlay));
				BufferedImage icoImg = findBestImage(protoImgs, ICO_SIZE, ICO_SIZE);
				
				if (icoImg != null)
				{
					Image ico = new Image(display, convertToSWT(icoImg));
					ImageData data = ico.getImageData();
					
					gc.drawImage(ico, 0, 0, data.width, data.height, left, top + (LINE_HEIGHT - ICO_SIZE) / 2, ICO_SIZE, ICO_SIZE);
					
					ico.dispose();
				}
				
				left += ICO_SIZE + SPACE;
			}
			
			top += LINE_HEIGHT + SPACE;
		}
		
		for (Plugin plugin : plugins)
		{
			List<PluginIcon> icons = plugin.getIcons();
			int rows = (int) ceil(icons.size() / (double) overlays.size());
			
			int left = BORDER;
			
			int textHeight = gc.stringExtent(plugin.getName()).y;
			gc.drawText(plugin.getName(), left, top + (LINE_HEIGHT - textHeight) / 2, true);
			
			for (int i = 0, icoNum = 0; i < rows && icoNum < icons.size(); i++)
			{
				left = BORDER + TEXT_MAX_WIDTH + SPACE;
				
				for (int j = 0; j < overlays.size() && icoNum < icons.size(); j++)
				{
					List<BufferedImage> imgs = ICODecoder.read(icons.get(icoNum++).getFile());
					BufferedImage icoImg = findBestImage(imgs, ICO_SIZE, ICO_SIZE);
					
					if (icoImg != null)
					{
						Image ico = new Image(display, convertToSWT(icoImg));
						ImageData data = ico.getImageData();
						
						gc.drawImage(ico, 0, 0, data.width, data.height, left, top + (LINE_HEIGHT - ICO_SIZE) / 2, ICO_SIZE, ICO_SIZE);
						
						ico.dispose();
					}
					
					left += ICO_SIZE + SPACE;
				}
				
				top += LINE_HEIGHT + SPACE;
			}
		}
		
		font.dispose();
		gc.dispose();
		
		ImageLoader loader = new ImageLoader();
		loader.data = new ImageData[] { img.getImageData() };
		loader.save(getFullPath(preview), SWT.IMAGE_PNG);
		
		img.dispose();
	}
	
	static ImageData convertToSWT(BufferedImage bufferedImage)
	{
		if (bufferedImage.getColorModel() instanceof DirectColorModel)
		{
			DirectColorModel colorModel = (DirectColorModel) bufferedImage.getColorModel();
			PaletteData palette = new PaletteData(colorModel.getRedMask(), colorModel.getGreenMask(), colorModel.getBlueMask());
			ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[4];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					raster.getPixel(x, y, pixelArray);
					int pixel = palette.getPixel(new RGB(pixelArray[0], pixelArray[1], pixelArray[2]));
					data.setPixel(x, y, pixel);
					data.setAlpha(x, y, pixelArray[3]);
				}
			}
			return data;
		}
		else if (bufferedImage.getColorModel() instanceof IndexColorModel)
		{
			IndexColorModel colorModel = (IndexColorModel) bufferedImage.getColorModel();
			int size = colorModel.getMapSize();
			byte[] reds = new byte[size];
			byte[] greens = new byte[size];
			byte[] blues = new byte[size];
			colorModel.getReds(reds);
			colorModel.getGreens(greens);
			colorModel.getBlues(blues);
			RGB[] rgbs = new RGB[size];
			for (int i = 0; i < rgbs.length; i++)
			{
				rgbs[i] = new RGB(reds[i] & 0xFF, greens[i] & 0xFF, blues[i] & 0xFF);
			}
			PaletteData palette = new PaletteData(rgbs);
			ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
			data.transparentPixel = colorModel.getTransparentPixel();
			WritableRaster raster = bufferedImage.getRaster();
			int[] pixelArray = new int[1];
			for (int y = 0; y < data.height; y++)
			{
				for (int x = 0; x < data.width; x++)
				{
					raster.getPixel(x, y, pixelArray);
					data.setPixel(x, y, pixelArray[0]);
				}
			}
			return data;
		}
		return null;
	}
	
	private static Point getMaxTextSize(List<String> protos, int fontHeight, boolean bold)
	{
		Display display = Display.getCurrent();
		Image img = new Image(display, 1, 1);
		
		GC gc = new GC(img);
		
		Font font = setFont(gc, fontHeight, bold);
		
		Point ret = new Point(0, 0);
		for (String proto : protos)
		{
			ret.x = max(ret.x, gc.stringExtent(proto).x);
			ret.y = max(ret.y, gc.stringExtent(proto).y);
		}
		
		font.dispose();
		gc.dispose();
		img.dispose();
		
		return ret;
	}
	
	private static Font setFont(GC gc, int height, boolean bold)
	{
		Font initialFont = gc.getFont();
		FontData[] fontData = initialFont.getFontData();
		for (int i = 0; i < fontData.length; i++)
		{
			fontData[i].setHeight(height);
			if (bold)
				fontData[i].setStyle(fontData[i].getStyle() | SWT.BOLD);
			else
				fontData[i].setStyle(fontData[i].getStyle() & ~SWT.BOLD);
		}
		Font font = new Font(Display.getCurrent(), fontData);
		gc.setFont(font);
		return font;
	}
	
	private static File getOutIco(String proto, Overlay overlay)
	{
		return new File(new File(cfg.paths.outPath, "status"), proto + "-" + overlay.getName() + ".ico");
	}
	
	private static void createIco(File outIco, File protoIco, Overlay overlay) throws IOException
	{
		List<BufferedImage> protoImgs = ICODecoder.read(protoIco);
		
		for (BufferedImage proto : protoImgs)
		{
			if (proto.getWidth() == 16 && proto.getHeight() == 16)
				overlay.blend(proto);
		}
		
		if (outIco.exists())
			outIco.delete();
		ICOEncoder.write(protoImgs, outIco);
	}
	
	private static void copy(File dest, File orig) throws IOException
	{
		if (!orig.exists())
			throw new FileNotFoundException(getFullPath(orig));
		if (dest.exists())
			dest.delete();
		
		Copy copy = new Copy();
		copy.setFile(orig);
		copy.setTofile(dest);
		copy.execute();
	}
	
	private static File getProtoIco(String name)
	{
		return new File(cfg.paths.protosPath, name + ".ico");
	}
	
	private static List<Overlay> readOverlays()
	{
		List<Overlay> ret = new ArrayList<Overlay>();
		ret.add(new Overlay(cfg, "offline"));
		ret.add(new Overlay(cfg, "online"));
		ret.add(new Overlay(cfg, "away"));
		ret.add(new Overlay(cfg, "dnd"));
		ret.add(new Overlay(cfg, "na"));
		ret.add(new Overlay(cfg, "occupied"));
		ret.add(new Overlay(cfg, "freechat"));
		ret.add(new Overlay(cfg, "invisible"));
		ret.add(new Overlay(cfg, "onthephone"));
		ret.add(new Overlay(cfg, "outtolunch"));
		return ret;
	}
	
	private static List<String> readProtos()
	{
		File[] files = cfg.paths.protosPath.listFiles(new FileFilter() {
			public boolean accept(File pathname)
			{
				return pathname.getName().endsWith(".ico");
			}
		});
		List<String> ret = new ArrayList<String>();
		for (int i = 0; i < files.length; i++)
		{
			File f = files[i];
			String name = f.getName();
			if (name.indexOf('-') >= 0)
				continue;
			ret.add(name.substring(0, name.length() - 4));
		}
		return ret;
	}
	
	private static List<Plugin> readPlugins()
	{
		File[] files = cfg.paths.pluginsPath.listFiles(new FileFilter() {
			public boolean accept(File pathname)
			{
				return pathname.isDirectory() && !".".equals(pathname) && !"..".equals(pathname);
			}
		});
		List<Plugin> ret = new ArrayList<Plugin>();
		for (int i = 0; i < files.length; i++)
		{
			File f = files[i];
			
			String name = f.getName();
			
			Plugin p = new Plugin(name, f);
			if (p.getIcons().size() <= 0)
				continue;
			
			ret.add(p);
		}
		return ret;
	}
	
	private static void initVelocity()
	{
		try
		{
			Properties props = new Properties();
			props.put("runtime.log.logsystem.class", VelocityLogger.class.getName());
			props.put("file.resource.loader.class", VelocityResourceLoader.class.getName());
			props.put("velocimacro.permissions.allow.inline.local.scope", "true");
			Velocity.init(props);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			throw new RuntimeException(e);
		}
	}
	
	private static void buildDll(Plugin plugin)
	{
		File dir = getPluginsDir();
		if (!dir.exists() && !dir.mkdirs())
			throw new IllegalStateException("Could not create the output path");
		
		File dllFile = new File(dir, plugin.getName() + ".dll");
		if (dllFile.exists())
			dllFile.delete();
		
		File rcFile = new File(dir, plugin.getName() + ".rc");
		
		mergeTemplate(plugin, rcFile);
		
		String rc = getFullPath(rcFile);
		rc = rc.substring(0, rc.length() - 3);
		
		File utils = new File("utils");
		File buildme = new File(utils, "buildme.bat");
		if (!buildme.exists())
			throw new IllegalStateException("Could not find utils dir");
		
		ExecTask exec = new ExecTask();
		exec.setExecutable(getFullPath(buildme));
		exec.createArg().setValue(rc);
		exec.setDir(utils);
		exec.setFailonerror(true);
		exec.setLogError(true);
		exec.execute();
	}
	
	private static File getPluginsDir()
	{
		return new File(cfg.paths.outPath, "plugins");
	}
	
	private static void mergeTemplate(Plugin plugin, File file)
	{
		File templateName = new File("utils/plugin.rc.vm");
		try
		{
			String pluginName = plugin.getName();
			
			VelocityContext context = new VelocityContext();
			context.put("plugin", encode(pluginName));
			context.put("description", encode(cfg.dllInfo.description.replace("%proto%", pluginName)));
			context.put("name", encode(cfg.dllInfo.name.replace("%proto%", pluginName)));
			context.put("copyright", encode(cfg.dllInfo.copyright.replace("%proto%", pluginName)));
			
			String[] version = cfg.dllInfo.version.split("\\.");
			for (int j = 0; j < 4; j++)
				context.put("v" + (j + 1), encode(Integer.toString(version.length >= j ? toInt(version[j]) : 0)));
			
			List<VelocityIcon> icons = new ArrayList<VelocityIcon>();
			for (PluginIcon icon : plugin.getIcons())
				icons.add(new VelocityIcon(Integer.toString(icon.getId()), encode(getFullPath(icon.getFile()))));
			context.put("icons", icons);
			
			Template template = Velocity.getTemplate(getFullPath(templateName));
			
			File parentFile = file.getParentFile();
			if (!parentFile.exists() && !parentFile.mkdirs())
				throw new IllegalStateException("Could not create the output path");
			
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "ISO-8859-1"));
			try
			{
				template.merge(context, writer);
			}
			finally
			{
				writer.close();
			}
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
	}
	
	public static class VelocityIcon
	{
		private final String id;
		private final String file;
		
		public VelocityIcon(String id, String file)
		{
			this.id = id;
			this.file = file;
		}
		
		public String getId()
		{
			return id;
		}
		
		public String getFile()
		{
			return file;
		}
	}
	
	private static void buildDll(String proto, List<Overlay> overlays) throws IOException, InterruptedException
	{
		File dir = new File(cfg.paths.outPath, "status");
		if (!dir.exists() && !dir.mkdirs())
			throw new IllegalStateException("Could not create the output path");
		
		File dllFile = new File(dir, proto + ".dll");
		if (dllFile.exists())
			dllFile.delete();
		
		File rcFile = new File(dir, proto + ".rc");
		
		mergeTemplate(proto, overlays, rcFile);
		
		String rc = getFullPath(rcFile);
		rc = rc.substring(0, rc.length() - 3);
		
		File utils = new File("utils");
		File buildme = new File(utils, "buildme.bat");
		if (!buildme.exists())
			throw new IllegalStateException("Could not find utils dir");
		
		ExecTask exec = new ExecTask();
		exec.setExecutable(getFullPath(buildme));
		exec.createArg().setValue(rc);
		exec.setDir(utils);
		exec.setFailonerror(true);
		exec.setLogError(true);
		exec.execute();
	}
	
	private static void mergeTemplate(String proto, List<Overlay> overlays, File file)
	{
		File templateName = new File("utils/autobuild.rc.vm");
		try
		{
			VelocityContext context = new VelocityContext();
			context.put("proto", encode(proto));
			context.put("description", encode(cfg.dllInfo.description.replace("%proto%", proto)));
			context.put("name", encode(cfg.dllInfo.name.replace("%proto%", proto)));
			context.put("copyright", encode(cfg.dllInfo.copyright.replace("%proto%", proto)));
			
			String[] version = cfg.dllInfo.version.split("\\.");
			for (int j = 0; j < 4; j++)
				context.put("v" + (j + 1), encode(Integer.toString(version.length >= j ? toInt(version[j]) : 0)));
			
			for (Overlay overlay : overlays)
				context.put(overlay.getName(), encode(getFullPath(getOutIco(proto, overlay))));
			
			Template template = Velocity.getTemplate(getFullPath(templateName));
			
			File parentFile = file.getParentFile();
			if (!parentFile.exists() && !parentFile.mkdirs())
				throw new IllegalStateException("Could not create the output path");
			
			BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file), "ISO-8859-1"));
			try
			{
				template.merge(context, writer);
			}
			finally
			{
				writer.close();
			}
		}
		catch (Exception e)
		{
			throw new RuntimeException(e);
		}
	}
	
	private static int toInt(String str)
	{
		try
		{
			return Integer.parseInt(str);
		}
		catch (NumberFormatException e)
		{
			return 0;
		}
	}
	
	private static String encode(String str)
	{
		return str.replace("\\", "\\\\").replace("\"", "\\\\\"");
	}
}
